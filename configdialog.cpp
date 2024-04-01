#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>

#include <algorithm>

#include "configdialog.h"
#include "yokscr.h"
#include "resourcew.h"

ConfigDialog::ConfigDialog(HWND dialog)
	: m_dialog(dialog), m_current_config(Registry::get_config()) {
	HWND pattern_box = GetDlgItem(m_dialog, IDC_YONK_PATTERN);
	for (const auto &entry : pattern_strings) {
		// They saw in the API great awkwardness...
		// So they solved the problem by - well, get this -
		// Wrapping the function they could have better designed
		// In a _macro_ that hides all its dubious lies...
		ComboBox_AddString(pattern_box, entry.second.c_str());
	}

	HWND palette_box = GetDlgItem(m_dialog, IDC_YONK_PALETTE);
	for (const auto &entry : palette_strings) {
		ComboBox_AddString(palette_box, entry.second.c_str());
	}

	refresh();
}

void ConfigDialog::save() {
	Registry registry;

	for (const auto &opt : Cfg::All) {
		registry.write(opt.name, m_current_config[opt]);
	}
}

BOOL ConfigDialog::command(WPARAM wparam, LPARAM lparam) {
	switch (LOWORD(wparam)) {
		case IDOK:
			save();
			[[fallthrough]];
		case IDCANCEL: {
			EndDialog(m_dialog, LOWORD(wparam) == IDOK);
			return TRUE;
		}
		case IDC_DEFAULTS_BUTTON: {
			for (const auto &opt : Cfg::All) {
				m_current_config[opt] = opt.default_;
			}
			refresh();
			return FALSE;
		}
		case IDC_PALETTE_CUSTOMIZE: {
			DialogBox(
				NULL,
				MAKEINTRESOURCE(DLG_PALETTE_CUSTOMIZER),
				m_dialog,
				(DLGPROC) ScreenSaverPaletteCustomizeDialog
			);
			break;
		}
		case IDC_YONK_PATTERN: {
			if (HIWORD(wparam) == CBN_SELENDOK) {
				return combobox_changed((HWND) lparam, IDC_YONK_PATTERN);
			}
			break;
		}
		case IDC_YONK_PALETTE: {
			if (HIWORD(wparam) == CBN_SELENDOK) {
				return combobox_changed((HWND) lparam, IDC_YONK_PALETTE);
			}
			break;
		}
		case IDC_PATTERN_FIX: {
			if (HIWORD(wparam) == BN_CLICKED) {
				return checkbox_checked(wparam, (HWND) lparam, Cfg::IsPatternFixed);
			}
			break;
		}
		case IDC_PLAY_OVER_DESKTOP: {
			if (HIWORD(wparam) == BN_CLICKED) {
				return checkbox_checked(wparam, (HWND) lparam, Cfg::PlayOverDesktop);
			}
			break;
		}
		case IDC_CUSTOM_PALETTES_CHECK: {
			if (HIWORD(wparam) == BN_CLICKED) {
				return checkbox_checked(wparam, (HWND) lparam, Cfg::UseCustomPalettes);
			}
			break;
		}
		case IDC_TRAILS_ENABLED: {
			if (HIWORD(wparam) == BN_CLICKED) {
				return checkbox_checked(wparam, (HWND) lparam, Cfg::TrailsEnabled);
			}
			break;
		}
	}

	return FALSE;
}

BOOL ConfigDialog::slider_changed(WPARAM wparam, HWND slider) {
	auto value = SendMessage(slider, TBM_GETPOS, 0, 0);

	auto opt = *std::find_if(Cfg::All.begin(), Cfg::All.end(), [&](const auto &opt) {
		return opt.dialog_control_id == GetDlgCtrlID(slider);
	});

	m_current_config[opt] = decodef(value);

	return FALSE;
}

BOOL ConfigDialog::combobox_changed(HWND combobox, int option_type) {
	wchar_t str[1 << 6];
	ComboBox_GetText(combobox, str, 1 << 6);
	std::wstring option_name(str);

	switch (option_type) {
		case IDC_YONK_PATTERN:
			m_current_config[Cfg::Pattern] = (double) reverse_lookup(pattern_strings, option_name);
			break;
		case IDC_YONK_PALETTE:
			m_current_config[Cfg::Palette] = (double) reverse_lookup(palette_strings, option_name);
			break;
	}

	return FALSE;
}

BOOL ConfigDialog::checkbox_checked(WPARAM wparam, HWND checkbox, const Cfg::Definition &option) {
	bool checked = Button_GetCheck(checkbox) == BST_CHECKED;
	m_current_config[option] = checked ? 1.0 : 0.0;
	refresh();
	return FALSE;
}

// Continuous sliders use integers only...
// What Redmond designer responds to this folly?!
long long ConfigDialog::encodef(double value) {
	return cast<long long>(value * 8.0);
}

double ConfigDialog::decodef(long long value) {
	return value / 8.0;
}

void ConfigDialog::refresh() {
	HWND version_label = GetDlgItem(m_dialog, IDC_VERSION_LABEL);
	std::wstring version_label_text = L"yokscr version " + YOKSCR_VERSION;
	SendMessage(version_label, WM_SETTEXT, NULL, (LPARAM) version_label_text.c_str());

	for (const auto &opt : Cfg::All) {
		HWND slider = GetDlgItem(m_dialog, opt.dialog_control_id);

		SendMessage(slider, TBM_SETRANGEMIN, TRUE, encodef(opt.range.first));
		SendMessage(slider, TBM_SETRANGEMAX, TRUE, encodef(opt.range.second));
		SendMessage(slider, TBM_SETPOS, TRUE, encodef(m_current_config[opt]));
	}

	// these should probably be put somewhere else at some point
	if (m_current_config[Cfg::Pattern] < 0
	 || m_current_config[Cfg::Pattern] >= (int) _PATTERN_COUNT) {
		m_current_config[Cfg::Pattern] = 0;
	}
	if (m_current_config[Cfg::Palette] < 0
	 || m_current_config[Cfg::Palette] >= (int) PaletteGroup::_PALETTE_OPTION_COUNT) {
		m_current_config[Cfg::Palette] = 0;
	}

	HWND pattern_box = GetDlgItem(m_dialog, IDC_YONK_PATTERN);
	ComboBox_SelectString(pattern_box, -1, pattern_strings.at(
		(PatternName) m_current_config[Cfg::Pattern]).c_str()
	);

	HWND palette_box = GetDlgItem(m_dialog, IDC_YONK_PALETTE);
	ComboBox_SelectString(palette_box, -1, palette_strings.at(
		(PaletteGroup) m_current_config[Cfg::Palette]).c_str()
	);

	bool is_pattern_fixed = m_current_config[Cfg::IsPatternFixed] == 1.0;
	HWND pattern_interval_slider = GetDlgItem(m_dialog, IDC_PATTERN_CHANGE_INTERVAL);
	HWND pattern_fixed_check = GetDlgItem(m_dialog, IDC_PATTERN_FIX);
	Button_SetCheck(pattern_fixed_check, is_pattern_fixed);
	EnableWindow(pattern_interval_slider, !is_pattern_fixed);

	bool is_playing_over_desktop = m_current_config[Cfg::PlayOverDesktop] == 1.0;
	HWND play_over_desktop_check = GetDlgItem(m_dialog, IDC_PLAY_OVER_DESKTOP);
	Button_SetCheck(play_over_desktop_check, is_playing_over_desktop);

	bool are_trails_enabled = m_current_config[Cfg::TrailsEnabled] == 1.0;
	HWND trail_length_slider = GetDlgItem(m_dialog, IDC_TRAIL_LENGTH);
	HWND trail_space_slider = GetDlgItem(m_dialog, IDC_TRAIL_SPACE);
	HWND trails_enabled_check = GetDlgItem(m_dialog, IDC_TRAILS_ENABLED);
	Button_SetCheck(trails_enabled_check, are_trails_enabled);
	EnableWindow(trail_length_slider, are_trails_enabled);
	EnableWindow(trail_space_slider, are_trails_enabled);

	bool using_custom_palettes = m_current_config[Cfg::UseCustomPalettes] == 1.0;
	HWND palette_selector = GetDlgItem(m_dialog, IDC_YONK_PALETTE);
	HWND palette_customize_button = GetDlgItem(m_dialog, IDC_PALETTE_CUSTOMIZE);
	HWND custom_palettes_check = GetDlgItem(m_dialog, IDC_CUSTOM_PALETTES_CHECK);
	EnableWindow(palette_selector, !using_custom_palettes);
	EnableWindow(palette_customize_button, using_custom_palettes);
	Button_SetCheck(custom_palettes_check, using_custom_palettes);
}

PaletteCustomizeDialog::PaletteCustomizeDialog(HWND dialog)
	: m_dialog(dialog),
	// BITMAP11 is lksix - we use this one because it shows
	// off every color in the palette.
	m_preview_bitmap(Bitmaps::load_raw_resource(IDB_BITMAP11)),
	// The "Friend" palette makes a good, netural-toned default.
	m_current_palette(*Palettes::Friend.data) {
	auto all_palettes = m_palette_repo.get_all_custom_palettes();
	if (!all_palettes.empty()) {
		m_current_palette = {
			.data = *all_palettes[0].data,
			.name = all_palettes[0].name
		};
	} else {
		m_current_palette = {};
	}

	refresh();
	refresh_palette_list();
}

BOOL PaletteCustomizeDialog::command(WPARAM wparam, LPARAM lparam) {
	switch (LOWORD(wparam)) {
		case IDOK: {
			save_current_palette();
			EndDialog(m_dialog, true);
			return TRUE;
		}
		case IDCANCEL: {
			EndDialog(m_dialog, true);
			return TRUE;
		}
		case IDC_PALDLG_SCALE_COLOR:
		case IDC_PALDLG_SCALE_HIGHLIGHT_COLOR:
		case IDC_PALDLG_SCALE_SHADOW_COLOR:
		case IDC_PALDLG_HORN_COLOR:
		case IDC_PALDLG_HORN_SHADOW_COLOR:
		case IDC_PALDLG_EYE_COLOR:
		case IDC_PALDLG_WHITES_COLOR: {
			int palette_index = palette_index_for_control(LOWORD(wparam));
			get_and_save_color(palette_index);
			break;
		}
		case IDC_PALDLG_PALETTE_LIST: {
			switch (HIWORD(wparam)) {
				case LBN_SELCHANGE: {
					update_current_palette();
					refresh();
					break;
				}
			}
			break;
		}
		case IDC_PALDLG_DUPE_PALETTE:
		case IDC_PALDLG_NEW_PALETTE: {
			std::wstring *name = (std::wstring *) DialogBox(
				NULL,
				MAKEINTRESOURCE(DLG_NEW_CUSTOM_PALETTE),
				m_dialog,
				(DLGPROC) ScreenSaverNewCustomPaletteDialog
			);

			if (name == nullptr) {
				break;
			}

			auto new_data = LOWORD(wparam) == IDC_PALDLG_DUPE_PALETTE
				? m_current_palette->data
				: *DefaultPalette.data;

			m_current_palette = {
				.data = new_data,
				.name = *name
			};

			save_current_palette();
			refresh_palette_list();

			delete name;

			break;
		}
		case IDC_PALDLG_DELETE_PALETTE: {
			delete_current_palette();
			refresh_palette_list();
			break;
		}
		case IDC_PALDLG_IMPORT_EXPORT_PALETTES: {
			std::wstring *exported_palettes = new std::wstring(export_palettes());
			std::wstring *palettes = (std::wstring *) DialogBoxParam(
				NULL,
				MAKEINTRESOURCE(DLG_IMPORT_EXPORT_PALETTES),
				m_dialog,
				(DLGPROC) ScreenSaverImportExportPalettesDialog,
				(LPARAM) exported_palettes
			);

			if (palettes == nullptr) {
				break;
			}

			import_palettes(palettes);
			break;
		}
	}

	return FALSE;
}

HBRUSH PaletteCustomizeDialog::handle_color_button_message(WPARAM wparam, LPARAM lparam) {
	HWND button = (HWND) lparam;
	int button_id = GetDlgCtrlID(button);

	if ((button_id != IDC_PALDLG_SCALE_COLOR
		&& button_id != IDC_PALDLG_SCALE_HIGHLIGHT_COLOR
		&& button_id != IDC_PALDLG_SCALE_SHADOW_COLOR
		&& button_id != IDC_PALDLG_HORN_COLOR
		&& button_id != IDC_PALDLG_HORN_SHADOW_COLOR
		&& button_id != IDC_PALDLG_EYE_COLOR
		&& button_id != IDC_PALDLG_WHITES_COLOR)
	 || !m_current_palette
	) {
		return FALSE;
	}

	int palette_index = palette_index_for_control(button_id);
	Color color = m_current_palette->data[palette_index];

	HBRUSH brush = CreateSolidBrush(RGB(
		std::get<RED>(color),
		std::get<GREEN>(color),
		std::get<BLUE>(color)
	));

	// This may be a resource leak because brushes are supposed to be freed
	// after their use, but I have no idea how to get a reference to this brush
	// back after it's been used. So, just let it leak?
	return brush;
}

void PaletteCustomizeDialog::refresh() {
	std::vector<HWND> controls_to_disable = {
		GetDlgItem(m_dialog, IDC_PALDLG_DELETE_PALETTE),
		GetDlgItem(m_dialog, IDC_PALDLG_SCALE_COLOR),
		GetDlgItem(m_dialog, IDC_PALDLG_SCALE_HIGHLIGHT_COLOR),
		GetDlgItem(m_dialog, IDC_PALDLG_SCALE_SHADOW_COLOR),
		GetDlgItem(m_dialog, IDC_PALDLG_HORN_COLOR),
		GetDlgItem(m_dialog, IDC_PALDLG_HORN_SHADOW_COLOR),
		GetDlgItem(m_dialog, IDC_PALDLG_EYE_COLOR),
		GetDlgItem(m_dialog, IDC_PALDLG_WHITES_COLOR),
		GetDlgItem(m_dialog, IDC_PALDLG_DUPE_PALETTE),
	};

	auto all_palettes = m_palette_repo.get_all_custom_palettes();
	for (HWND control : controls_to_disable) {
		EnableWindow(control, !all_palettes.empty());
	}

	if (m_current_palette) {
		apply_palette_to_preview(m_current_palette->data);
	} else {
		apply_palette_to_preview(*DisabledPalette.data);
	}

	SendDlgItemMessage(
		m_dialog,
		IDC_PALDLG_PREVIEW,
		STM_SETIMAGE,
		IMAGE_BITMAP,
		(LPARAM) m_preview_bitmap
	);

	// Force the color buttons to re-paint, they won't on their own
	InvalidateRect(m_dialog, NULL, TRUE);
}

void PaletteCustomizeDialog::refresh_palette_list() {
	HWND palette_list = GetDlgItem(m_dialog, IDC_PALDLG_PALETTE_LIST);
	ListBox_ResetContent(palette_list);

	auto all_palettes = m_palette_repo.get_all_custom_palettes();
	for (const auto &palette : all_palettes) {
		ListBox_AddString(palette_list, palette.name.c_str());
	}

	if (m_current_palette) {
		ListBox_SelectString(palette_list, -1, m_current_palette->name.c_str());
	}

	refresh();
}

void PaletteCustomizeDialog::update_current_palette() {
	save_current_palette();

	auto all_palettes = m_palette_repo.get_all_custom_palettes();
	size_t palette_index = SendDlgItemMessage(
		m_dialog,
		IDC_PALDLG_PALETTE_LIST,
		LB_GETCURSEL,
		0,
		0
	);

	wchar_t buffer[MaxPaletteNameSize] { L'\0' };
	SendDlgItemMessage(
		m_dialog,
		IDC_PALDLG_PALETTE_LIST,
		LB_GETTEXT,
		palette_index,
		(LPARAM) buffer
	);
	std::wstring selected = buffer;

	m_current_palette = {
		.data = *std::find_if(all_palettes.begin(), all_palettes.end(), [&](const Palettes::Definition &palette) {
			return palette.name == selected;
		})->data,
		.name = selected,
	};
}

void PaletteCustomizeDialog::save_current_palette() {
	if (m_current_palette) {
		m_palette_repo.set_palette(m_current_palette->name, m_current_palette->data);
	}
}

void PaletteCustomizeDialog::delete_current_palette() {
	if (!m_current_palette) {
		return;
	}

	m_palette_repo.remove_palette(m_current_palette->name);

	auto all_palettes = m_palette_repo.get_all_custom_palettes();
	if (!all_palettes.empty()) {
		m_current_palette = {
			.data = *all_palettes[0].data,
			.name = all_palettes[0].name,
		};
	} else {
		m_current_palette = {};
	}
}

void PaletteCustomizeDialog::apply_palette_to_preview(const PaletteData &palette) {
	HDC memory_context = CreateCompatibleDC(GetDC(m_dialog));

	SelectBitmap(memory_context, m_preview_bitmap);
	RGBQUAD colors[_PALETTE_SIZE];

	// I REALLY don't want to go down a rabbit hole of figuring out how to make this bitmap
	// actually transparent (it's an absolute miracle any of this works _at all_!), so we
	// just guess what the base color of the dialog is to fake transparency.
	// We could get the actual color from a system brush, but that sounds like effort!
	colors[PI_TRANSPARENT] = {
		.rgbBlue = 240,
		.rgbGreen = 240,
		.rgbRed = 240
	};

	for (int i = 1; i < _PALETTE_SIZE; i++) {
		colors[i] = {
			.rgbBlue = std::get<BLUE>(palette[i]),
			.rgbGreen = std::get<GREEN>(palette[i]),
			.rgbRed = std::get<RED>(palette[i]),
		};
	}

	SetDIBColorTable(
		memory_context,
		0,
		_PALETTE_SIZE,
		colors
	);

	DeleteDC(memory_context);
}

int PaletteCustomizeDialog::palette_index_for_control(int color_button_control_id) {
	switch (color_button_control_id) {
		default:
		case IDC_PALDLG_SCALE_COLOR: return PI_SCALES;
		case IDC_PALDLG_SCALE_HIGHLIGHT_COLOR: return PI_SCALES_HIGHLIGHT;
		case IDC_PALDLG_SCALE_SHADOW_COLOR: return PI_SCALES_SHADOW;
		case IDC_PALDLG_HORN_COLOR: return PI_HORNS;
		case IDC_PALDLG_HORN_SHADOW_COLOR: return PI_HORNS_SHADOW;
		case IDC_PALDLG_EYE_COLOR: return PI_EYE;
		case IDC_PALDLG_WHITES_COLOR: return PI_WHITES;
	}
}

void PaletteCustomizeDialog::get_and_save_color(int palette_index) {
	if (!m_current_palette) {
		return;
	}

	static COLORREF custom_colors[16];

	Color current_color = m_current_palette->data[palette_index];
	CHOOSECOLOR color_options = {
		.lStructSize = sizeof(CHOOSECOLOR),
		.hwndOwner = m_dialog,
		.hInstance = NULL,
		.rgbResult = RGB(std::get<RED>(current_color), std::get<GREEN>(current_color), std::get<BLUE>(current_color)),
		.lpCustColors = custom_colors,
		.Flags = CC_FULLOPEN | CC_ENABLETEMPLATE | CC_ENABLEHOOK,
		.lCustData = 0,
		.lpfnHook = (LPCCHOOKPROC) CustomColorDialog,
		.lpTemplateName = MAKEINTRESOURCE(DLG_CUSTOM_CHOOSECOLOR),
	};

	bool color_choice_successful = ChooseColor(&color_options);
	if (color_choice_successful) {
		m_current_palette->data[palette_index] = {
			GetRValue(color_options.rgbResult),
			GetGValue(color_options.rgbResult),
			GetBValue(color_options.rgbResult),
			255
		};

		// fuck it
		save_current_palette();
		refresh();
	}
}

std::wstring PaletteCustomizeDialog::export_palettes() {
	auto all_palettes = m_palette_repo.get_all_custom_palettes();
	std::wstring palettes_string = L"";

	for (const auto &palette : all_palettes) {
		auto name = palette.name;
		auto *data = palette.data;

		palettes_string.append(palette.name + L": ");
		std::wstring colors = m_palette_repo.serialize(*data) + L";";
		// Add extra "fake" colors so that a palette definition is guaranteed to be longer than the maximum allowed name length
		while (colors.length() < PaletteCustomizeDialog::MaxPaletteNameSize) {
			colors.append(L"#000000;");
		}
		palettes_string.append(colors);
		palettes_string.append(L"\r\n");
	}
	palettes_string.append(L"~");

	return palettes_string;
}

void PaletteCustomizeDialog::import_palettes(std::wstring *palettes) {
	using namespace std;

	const static wstring valid_chars = L"qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789-_!?:";
	const static wstring valid_hex_chars = L"0123456789abcdefABCDEF";
	const static size_t colors_amount = max(7, (size_t)ceil(PaletteCustomizeDialog::MaxPaletteNameSize / 8.0));

	size_t palettes_amount = 0;
	size_t current_index = 0;
	bool graceful_exit = false;
	size_t count_invalid = 0;

	const enum class Errors {
		none = 0,
		name_too_short = 1 << 0,
		name_too_long = 1 << 1,
		name_invalid_characters = 1 << 2,
		colors_too_few = 1 << 3,
		colors_incorrect_length = 1 << 4,
		colors_invalid_characters = 1 << 5,
	};

	vector<wstring> names;
	vector<vector<wstring>> colors;
	vector<uint8_t> is_valid;

	for (; palettes->find_first_of(valid_chars, current_index) != wstring::npos && palettes->find_first_of(L':', current_index) != wstring::npos; palettes_amount++) {
		graceful_exit = (palettes->find_first_of(L'~', current_index) != wstring::npos);

		wstring current_name = L"";
		vector<wstring> current_colors;
		for (size_t i = 0; i < colors_amount; i++) {
			current_colors.push_back(L";");
		}
		uint8_t current_is_valid = 0;

		current_index = palettes->find_first_of(valid_chars, current_index);
		current_name = palettes->substr(current_index, palettes->find_first_of(L':', current_index) - current_index);
		if (current_name.length() < PaletteCustomizeDialog::MinPaletteNameSize) {
			current_is_valid |= static_cast<uint8_t>(Errors::name_too_short);
		}
		if (current_name.length() >= PaletteCustomizeDialog::MaxPaletteNameSize) {
			current_is_valid |= static_cast<uint8_t>(Errors::name_too_long);
		}
		if (current_name.find_first_not_of(valid_chars + L' ') != wstring::npos) {
			current_is_valid |= static_cast<uint8_t>(Errors::name_invalid_characters);
		}

		current_index = palettes->find_first_of(L':', current_index) + 1;
		for (int i = 0; i < colors_amount; i++) {
			if (palettes->find_first_of(L':', current_index) < palettes->find_first_of(L'#', current_index)) {
				current_is_valid |= static_cast<uint8_t>(Errors::colors_too_few);
				break;
			}

			if (palettes->find_first_of(L'#', current_index) == wstring::npos) {
				current_is_valid |= static_cast<uint8_t>(Errors::colors_too_few);
				break;
			}
			current_index = palettes->find_first_of(L'#', current_index) + 1;
			current_colors[i] = palettes->substr(current_index, palettes->find_first_of(L';', current_index) - current_index);
			if (palettes->find_first_of(L';', current_index) == wstring::npos) {
				current_is_valid |= static_cast<uint8_t>(Errors::colors_too_few);
				break;
			}
			current_index = palettes->find_first_of(L';', current_index) + 1;

			if (current_colors[i].length() != 6) {
				current_is_valid |= static_cast<uint8_t>(Errors::colors_incorrect_length);
			}
			if (current_colors[i].find_first_not_of(valid_hex_chars) != wstring::npos) {
				current_is_valid |= static_cast<uint8_t>(Errors::colors_invalid_characters);
			}
		}

		if (current_is_valid != static_cast<uint8_t>(Errors::none)) {
			count_invalid++;
		}

		names.push_back(current_name);
		colors.push_back(current_colors);
		is_valid.push_back(current_is_valid);
		printf("");
	}

	bool import = true;
	if (count_invalid <= 1 && !graceful_exit && (count_invalid == 0 || *is_valid.rbegin() != static_cast<uint8_t>(Errors::none))) {
		wstring error_info = L"No terminating character (~) detected. One or more palettes may be missing.\nLast detected palette: " + *(names.rbegin() + count_invalid);
		error_info += L"\n\nWould you like to import the palettes anyway?\n\nThe following palettes can be imported:";
		for (size_t i = 0; i < palettes_amount; i++) {
			if (is_valid[i] == static_cast<uint8_t>(Errors::none)) {
				error_info += L" " + names[i] + L",";
			}
		}
		*error_info.rbegin() = L'.';

		int num = MessageBox(
			NULL,
			error_info.c_str(),
			L"Palette Customizer",
			MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2
		);
		import = (num == IDYES);
	}
	else if (count_invalid > 0) {
		wstring error_info = L"One or more palettes have errors:\n";
		for (size_t i = 0, invalid = 0; i < palettes_amount; i++) {
			if (is_valid[i] != static_cast<uint8_t>(Errors::none)) {
				invalid++;
				wstring name = names[i];
				while (name.find_first_of(L'\r') != wstring::npos) {
					name.replace(name.find_first_of(L'\r'), 1, L"\\r");
				}
				while (name.find_first_of(L'\n') != wstring::npos) {
					name.replace(name.find_first_of(L'\n'), 1, L"\\n");
				}
				error_info += name + L":";
				if ((is_valid[i] & static_cast<uint8_t>(Errors::name_too_short)) != static_cast<uint8_t>(Errors::none)) {
					error_info += L" Name is too short,";
				}
				if ((is_valid[i] & static_cast<uint8_t>(Errors::name_too_long)) != static_cast<uint8_t>(Errors::none)) {
					error_info += L" Name is too long,";
				}
				if ((is_valid[i] & static_cast<uint8_t>(Errors::name_invalid_characters)) != static_cast<uint8_t>(Errors::none)) {
					size_t index = 0;
					wstring invalid_chars = L"";
					while (names[i].find_first_not_of(valid_chars + L' ', index) != wstring::npos) {
						index = names[i].find_first_not_of(valid_chars + L' ', index);
						if (invalid_chars.find_first_of(names[i][index]) == wstring::npos) {
							if (names[i][index] == L'\r') {
								invalid_chars += L"\\r";
							}
							else if (names[i][index] == L'\n') {
								invalid_chars += L"\\n";
							}
							else {
								invalid_chars += names[i][index];
							}
						}
						index++;
					}
					error_info += L" Name has invalid characters (" + invalid_chars + L"),";
				}
				if ((is_valid[i] & static_cast<uint8_t>(Errors::colors_too_few)) != static_cast<uint8_t>(Errors::none)) {
					int count = 0;
					for (wstring color : colors[i]) {
						if (color.find_first_of(L';') == wstring::npos) {
							count++;
						}
					}
					error_info += L" Too few colors defined (have " + to_wstring(count) + L", need " + to_wstring(colors_amount) + L"),";
				}
				if ((is_valid[i] & static_cast<uint8_t>(Errors::colors_incorrect_length)) != static_cast<uint8_t>(Errors::none)) {
					error_info += L" One or more colors were of incorrect length (found:";
					for (wstring color : colors[i]) {
						if (color.find_first_of(';') == wstring::npos) {
							error_info += L" " + to_wstring(color.length()) + L",";
						}
					}
					*error_info.rbegin() = L';';
					error_info += L" expected: 6),";
				}
				if ((is_valid[i] & static_cast<uint8_t>(Errors::colors_invalid_characters)) != static_cast<uint8_t>(Errors::none)) {
					wstring invalid_chars = L"";
					for (wstring color : colors[i]) {
						size_t index = 0;
						while (color.find_first_not_of(valid_hex_chars + L';', index) != wstring::npos) {
							index = color.find_first_not_of(valid_hex_chars + L';', index);
							if (invalid_chars.find_first_of(color[index]) == wstring::npos) {
								invalid_chars += color[index];
							}
							index++;
						}
					}
					error_info += L" Colors have invalid characters (" + invalid_chars + L"),";
				}

				if (invalid < count_invalid) {
					*error_info.rbegin() = L';';
					error_info += L"\n";
				}
				else {
					error_info.pop_back();
				}

				if (invalid >= 10 && invalid < count_invalid) {
					error_info += L"... + " + to_wstring(count_invalid - invalid) + L" more palettes";
					break;
				}
			}
		}
		

		if (count_invalid >= palettes_amount) {
			error_info += L"\n\nNo palettes can be imported.";
			MessageBox(
				NULL,
				error_info.c_str(),
				L"Palette Customizer",
				MB_OK | MB_ICONERROR
			);
			import = false;
		}
		else {
			error_info += L"\n\nWould you like to import the remaining palettes anyway?\n\nThe following palettes can be imported:";
			for (size_t i = 0; i < palettes_amount; i++) {
				if (is_valid[i] == static_cast<uint8_t>(Errors::none)) {
					error_info += L" " + names[i] + L",";
				}
			}
			*error_info.rbegin() = L'.';
			int num = MessageBox(
				NULL,
				error_info.c_str(),
				L"Palette Customizer",
				MB_YESNO | MB_ICONERROR | MB_DEFBUTTON2
			);
			import = (num == IDYES);
		}
	}
	else {
		wstring import_confirmation = L"The following palettes will be imported:";
		for (size_t i = 0; i < palettes_amount; i++) {
			if (is_valid[i] == static_cast<uint8_t>(Errors::none)) {
				import_confirmation += L" " + names[i] + L",";
			}
		}
		*import_confirmation.rbegin() = L'.';
		import_confirmation += L"\n\nConfirm palette import?";
		int num = MessageBox(
			NULL,
			import_confirmation.c_str(),
			L"Palette Customizer",
			MB_YESNO | MB_ICONINFORMATION
		);
		import = (num == IDYES);
	}

	if (import) {
		// TODO: Actually add import code
	}
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
	static ConfigDialog *cfg_dialog;

	switch (message) {
		case WM_INITDIALOG: {
			cfg_dialog = new ConfigDialog(dialog);
			return TRUE;
		}
		case WM_COMMAND: {
			return cfg_dialog->command(wparam, lparam);
		}
		case WM_HSCROLL: {
			HWND handle = (HWND) lparam;

			if (handle != NULL) {
				return cfg_dialog->slider_changed(wparam, handle);
			}
		}
	}

	return FALSE;
}

LRESULT CALLBACK ScreenSaverPaletteCustomizeDialog(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
	static PaletteCustomizeDialog *palette_dialog;

	switch (message) {
		case WM_INITDIALOG: {
			palette_dialog = new PaletteCustomizeDialog(dialog);
			return TRUE;
		} case WM_COMMAND: {
			return palette_dialog->command(wparam, lparam);
		} case WM_CTLCOLORBTN: {
			return (LRESULT) palette_dialog->handle_color_button_message(wparam, lparam);
		}
	}

	return FALSE;
}

LRESULT CALLBACK ScreenSaverNewCustomPaletteDialog(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
	switch (message) {
		case WM_INITDIALOG: {
			return TRUE;
		} case WM_COMMAND: {
			HWND name_input = GetDlgItem(dialog, IDC_PALDLG_NEW_PALETTE_NAME);

			wchar_t name_buffer[PaletteCustomizeDialog::MaxPaletteNameSize] { L'\0' };
			Edit_GetText(
				name_input,
				name_buffer,
				PaletteCustomizeDialog::MaxPaletteNameSize
			);
			name_buffer[PaletteCustomizeDialog::MaxPaletteNameSize - 1] = L'\0';

			switch (LOWORD(wparam)) {
				case IDOK: {
					auto *name = new std::wstring(name_buffer);
					EndDialog(dialog, (INT_PTR) name);
					return TRUE;
				} case IDCANCEL: {
					EndDialog(dialog, 0);
					return TRUE;
				} case IDC_PALDLG_NEW_PALETTE_NAME: {
					switch (HIWORD(wparam)) {
						case EN_CHANGE: {
							// Various special characters might break the registry parsing if they are saved.
							// The only catastrophe I can directly forsee is commas, but may as well just only allow
							// alphanumerics and a few special characters to be extra safe.
							static std::wstring valid_chars = L"qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789 -_!?:";
							std::wstring name = name_buffer;

							bool name_too_short = name.size() < PaletteCustomizeDialog::MinPaletteNameSize;
							bool name_has_invalid_chars = name.find_first_not_of(valid_chars) != std::wstring::npos;
							// There is no "name_too_long" check because names are 
							// automatically truncated due to the buffer size.

							HWND ok_button = GetDlgItem(dialog, IDOK);
							EnableWindow(ok_button, !name_too_short && !name_has_invalid_chars);
							break;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return FALSE;
}

LRESULT CALLBACK CustomColorDialog(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
	const static size_t max_hex_code_length = 7; // 6 digits + the pound sign (#)

	static auto color_from_hex_string = [&](const std::wstring &hex_code) -> std::optional<Color> {
		static const std::wstring valid_hex_digits = L"0123456789abcdefABCDEF";

		bool starts_with_pound = hex_code[0] == L'#';
		bool has_invalid_hex_digit = hex_code.find_first_not_of(valid_hex_digits) != std::wstring::npos;
		bool has_invalid_hex_digit_after_first = hex_code.find_first_not_of(valid_hex_digits, 1) != std::wstring::npos;

		bool is_valid_pound_code = (hex_code.size() == max_hex_code_length)
			&& (starts_with_pound && !has_invalid_hex_digit_after_first);
		bool is_valid_digits_code = (hex_code.size() == max_hex_code_length - 1)
			&& (!starts_with_pound && !has_invalid_hex_digit);

		if (!is_valid_pound_code && !is_valid_digits_code) {
			return {};
		}

		if (is_valid_pound_code) {
			auto red = std::stoi(hex_code.substr(1, 2), nullptr, 16);
			auto green = std::stoi(hex_code.substr(3, 2), nullptr, 16);
			auto blue = std::stoi(hex_code.substr(5, 2), nullptr, 16);

			return { { red, green, blue, 255 } };
		}

		if (is_valid_digits_code) {
			auto red = std::stoi(hex_code.substr(0, 2), nullptr, 16);
			auto green = std::stoi(hex_code.substr(2, 2), nullptr, 16);
			auto blue = std::stoi(hex_code.substr(4, 2), nullptr, 16);

			return { { red, green, blue, 255 } };
		}

		return {};
	};

	static UINT set_rgb_message_code = RegisterWindowMessage(SETRGBSTRING);

	switch (message) {
		case WM_INITDIALOG: {
			SendDlgItemMessage(
				dialog,
				IDC_COLORDLG_HEX_CODE,
				EM_SETLIMITTEXT,
				max_hex_code_length,
				0
			);
			break;
		}
		case WM_NOTIFY: {
			if (lparam != NULL) {
				NMHDR notification = *((NMHDR *) lparam);
				auto a = 0;
			}
			break;
		}
		case WM_COMMAND: {
			switch (LOWORD(wparam)) {
				case IDC_COLORDLG_HEX_CODE: {
					switch (HIWORD(wparam)) {
						case EN_CHANGE: {
							HWND hex_input = GetDlgItem(dialog, IDC_COLORDLG_HEX_CODE);
							wchar_t hex_code_buffer[max_hex_code_length + 1] { L'\0' };
							Edit_GetText(
								hex_input,
								hex_code_buffer,
								max_hex_code_length + 1
							);
							hex_code_buffer[max_hex_code_length] = L'\0';
							std::wstring hex_code = hex_code_buffer;

							auto color = color_from_hex_string(hex_code);
							if (!color) {
								return FALSE;
							}

							SendMessage(
								dialog,
								set_rgb_message_code,
								0,
								(LPARAM) RGB(std::get<RED>(*color), std::get<GREEN>(*color), std::get<BLUE>(*color))
							);
						}
					}
				}
			}
		}
	}

	return FALSE;
}

LRESULT CALLBACK ScreenSaverImportExportPalettesDialog(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
	HWND palettes_input = GetDlgItem(dialog, IDC_PALDLG_IMPORT_EXPORT_PALETTES);
	switch (message) {
		case WM_INITDIALOG: {
			std::wstring *exported_palettes = (std::wstring *) lparam;
			Edit_SetText(
				palettes_input,
				(wchar_t *) exported_palettes->c_str()
			);
			delete exported_palettes;
			return TRUE;
		} case WM_COMMAND: {
			std::unique_ptr<wchar_t> palettes_buffer(new wchar_t[1 << 16](L'\0'));
			Edit_GetText(
				palettes_input,
				palettes_buffer.get(),
				1 << 16
			);
			(palettes_buffer.get())[(1 << 16) - 1] = L'\0';

			switch (LOWORD(wparam)) {
				case IDOK: {
					auto *palettes = new std::wstring(palettes_buffer.get());
					EndDialog(dialog, (INT_PTR) palettes);
					return TRUE;
				} case IDCANCEL: {
					EndDialog(dialog, 0);
					return TRUE;
				} case IDRETRY: {
					Edit_SetText(
						palettes_input,
						(wchar_t *) lparam
					);
					return TRUE;
				} case IDC_PALDLG_IMPORT_EXPORT_PALETTES: {
					switch (HIWORD(wparam)) {
						case EN_CHANGE: {
							// Various special characters might break the registry parsing if they are saved.
							// The only catastrophe I can directly forsee is commas, but may as well just only allow
							// alphanumerics and a few special characters to be extra safe.
							static std::wstring valid_chars = L"qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789 -_!?:#;~\r\n";
							std::wstring palettes = palettes_buffer.get();

							bool palettes_too_short = palettes.size() < PaletteCustomizeDialog::MinPaletteNameSize + 65;
							bool palettes_has_invalid_chars = palettes.find_first_not_of(valid_chars) != std::wstring::npos;

							HWND ok_button = GetDlgItem(dialog, IDOK);
							EnableWindow(ok_button, !palettes_too_short && !palettes_has_invalid_chars);
							break;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return FALSE;
}
