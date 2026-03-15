#include "ui/wx/wx_game_config_dialog.h"

#include <wx/tokenzr.h>
#include <wx/msgdlg.h>

namespace ui::wx {

WxGameConfigDialog::WxGameConfigDialog(wxWindow* parent,
                                       EventHandler* event_handler,
                                       std::string game_name,
                                       model::GameConfigDescriptor descriptor)
    : event_handler_(event_handler),
      game_name_(std::move(game_name)),
      descriptor_(std::move(descriptor)) {
  Create(parent, wxID_ANY, "Configure " + game_name_, wxDefaultPosition,
         wxSize(600, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

  CreateButtons(wxOK | wxCANCEL);

  wxBookCtrlBase* book = GetBookCtrl();

  for (const auto& section : descriptor_.sections) {
    book->AddPage(CreateSectionPage(book, section), section.name);
  }

  Bind(wxEVT_BUTTON, &WxGameConfigDialog::OnOK, this, wxID_OK);

  LayoutDialog();
}

void WxGameConfigDialog::OnOK(wxCommandEvent& event) {
  (void)event;
  event_handler_->CommitGameConfig(game_name_, std::move(descriptor_));
  EndModal(wxID_OK);
}

wxPanel* WxGameConfigDialog::CreateSectionPage(wxWindow* parent,
                                               const model::GameConfigSection& section) {
  auto* panel = new wxPanel(parent);
  auto* sizer = new wxBoxSizer(wxVERTICAL);

  for (const auto& option : section.options) {
    if (option.type == model::GameConfigOptionType::kEditableList) {
      sizer->Add(new wxStaticText(panel, wxID_ANY, option.label + ":"), 0,
                 wxALL, 5);
      sizer->Add(CreateListControl(panel, option), 1, wxEXPAND | wxALL, 5);
    } else {
      auto* hsizer = new wxBoxSizer(wxHORIZONTAL);
      hsizer->Add(new wxStaticText(panel, wxID_ANY, option.label + ":"), 0,
                  wxALIGN_CENTER_VERTICAL | wxALL, 5);
      hsizer->Add(CreateOptionControl(panel, option), 1, wxEXPAND | wxALL, 5);
      sizer->Add(hsizer, 0, wxEXPAND);
    }

    if (!option.description.empty()) {
      auto* desc = new wxStaticText(panel, wxID_ANY, option.description);
      desc->SetFont(desc->GetFont().Italic());
      sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    }
  }

  panel->SetSizer(sizer);
  return panel;
}

wxWindow* WxGameConfigDialog::CreateOptionControl(wxWindow* parent,
                                                  const model::GameConfigOption& option) {
  switch (option.type) {
    case model::GameConfigOptionType::kString: {
      auto* ctrl = new wxTextCtrl(parent, wxID_ANY, option.value);
      ctrl->Bind(wxEVT_TEXT, [this, key = option.key](wxCommandEvent& event) {
        for (auto& section : descriptor_.sections) {
          for (auto& opt : section.options) {
            if (opt.key == key) {
              opt.value = event.GetString().ToStdString();
              return;
            }
          }
        }
      });
      return ctrl;
    }
    case model::GameConfigOptionType::kFilePath: {
      auto* ctrl = new wxFilePickerCtrl(parent, wxID_ANY, option.value,
                                        "Select file", "*.*");
      ctrl->Bind(
          wxEVT_FILEPICKER_CHANGED,
          [this, key = option.key](wxFileDirPickerEvent& event) {
            for (auto& section : descriptor_.sections) {
              for (auto& opt : section.options) {
                if (opt.key == key) {
                  opt.value = event.GetPath().ToStdString();
                  return;
                }
              }
            }
          });
      return ctrl;
    }
    case model::GameConfigOptionType::kDirectoryPath: {
      auto* ctrl = new wxDirPickerCtrl(parent, wxID_ANY, option.value,
                                       "Select directory");
      ctrl->Bind(
          wxEVT_DIRPICKER_CHANGED,
          [this, key = option.key](wxFileDirPickerEvent& event) {
            for (auto& section : descriptor_.sections) {
              for (auto& opt : section.options) {
                if (opt.key == key) {
                  opt.value = event.GetPath().ToStdString();
                  return;
                }
              }
            }
          });
      return ctrl;
    }
    default:
      return new wxStaticText(parent, wxID_ANY, "Unsupported option type");
  }
}

wxPanel* WxGameConfigDialog::CreateListControl(wxWindow* parent,
                                               const model::GameConfigOption& option) {
  auto* panel = new wxPanel(parent);
  auto* sizer = new wxBoxSizer(wxVERTICAL);

  auto* list = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                              wxLC_REPORT | wxLC_SINGLE_SEL);

  for (size_t i = 0; i < option.list_columns.size(); ++i) {
    list->InsertColumn(i, option.list_columns[i].name, wxLIST_FORMAT_LEFT,
                       option.list_columns[i].width);
  }

  for (const auto& item : option.list_items) {
    long index = list->InsertItem(list->GetItemCount(), item.fields[0]);
    for (size_t i = 1; i < item.fields.size(); ++i) {
      list->SetItem(index, i, item.fields[i]);
    }
    list->SetItemPtrData(index, reinterpret_cast<wxUIntPtr>(new std::string(item.id)));
  }

  sizer->Add(list, 1, wxEXPAND | wxALL, 0);

  auto* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  auto* add_btn = new wxButton(panel, wxID_ANY, "Add...");
  auto* remove_btn = new wxButton(panel, wxID_ANY, "Remove");

  btn_sizer->Add(add_btn, 0, wxALL, 5);
  btn_sizer->Add(remove_btn, 0, wxALL, 5);
  sizer->Add(btn_sizer, 0, wxALIGN_RIGHT);

  add_btn->Bind(wxEVT_BUTTON, [this, list, key = option.key, columns_size = option.list_columns.size()](wxCommandEvent&) {
    std::string prompt = "Enter fields (comma separated):";
    wxTextEntryDialog dlg(this, prompt, "Add Item");
    if (dlg.ShowModal() == wxID_OK) {
        wxString val = dlg.GetValue();
        std::vector<std::string> fields;
        wxStringTokenizer tokenizer(val, ",");
        while (tokenizer.HasMoreTokens()) {
            wxString token = tokenizer.GetNextToken();
            token.Trim(true).Trim(false);
            fields.push_back(token.ToStdString());
        }

        if (fields.size() >= columns_size) {
            // Find option in local descriptor
            for (auto& section : descriptor_.sections) {
              for (auto& opt : section.options) {
                if (opt.key == key) {
                  std::string id = fields.size() > 1 ? fields[1] : fields[0];
                  opt.list_items.push_back({id, fields});

                  // Update UI
                  long index = list->InsertItem(list->GetItemCount(), wxString::FromUTF8(fields[0]));
                  for (size_t i = 1; i < fields.size(); ++i) {
                      list->SetItem(index, i, wxString::FromUTF8(fields[i]));
                  }
                  list->SetItemPtrData(index, reinterpret_cast<wxUIntPtr>(new std::string(id)));
                  return;
                }
              }
            }
        } else {
            wxMessageBox("Not enough fields entered. Expected " + std::to_string(columns_size),
                         "Error", wxOK | wxICON_ERROR);
        }
    }
  });

  remove_btn->Bind(wxEVT_BUTTON, [this, list, key = option.key](wxCommandEvent&) {
    long selected = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selected != -1) {
        std::string* id = reinterpret_cast<std::string*>(list->GetItemData(selected));

        // Remove from local descriptor
        for (auto& section : descriptor_.sections) {
          for (auto& opt : section.options) {
            if (opt.key == key) {
              auto it = std::remove_if(opt.list_items.begin(), opt.list_items.end(),
                                       [id](const auto& item) { return item.id == *id; });
              opt.list_items.erase(it, opt.list_items.end());
              break;
            }
          }
        }

        delete id;
        list->DeleteItem(selected);
    }
  });

  panel->SetSizer(sizer);
  return panel;
}

}  // namespace ui::wx
