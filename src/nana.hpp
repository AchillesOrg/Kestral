#pragma once

#include <string>
#include <utility>
#include <vector>

namespace kestral {

struct NanaTemplate {
    std::string id;
    std::string title;
    std::string description;
    std::string filename;
    std::string code;
};

struct RawTtyGuard {
    bool active = false;
#ifndef _WIN32
    void* saved_state = nullptr;
#endif
    RawTtyGuard() = default;
    ~RawTtyGuard();
    void enter();
    void leave();
};

std::string kestral_home_dir();
std::string load_bg_mode();
std::string save_bg_mode(const std::string& mode);

std::pair<int, int> get_terminal_size();
bool stdin_stdout_are_tty();
std::string read_key();

struct MouseEvent {
    std::string kind; // "CLICK", "WHEEL_UP", "WHEEL_DOWN"
    int row = 0;
    int col = 0;
};
bool parse_mouse_event(const std::string& key, MouseEvent& out);
std::string footer_shortcut_hit(const std::string& footer, int col,
                                const std::vector<std::pair<std::string, std::string>>& mappings);

std::string strip_ansi(const std::string& s);
std::size_t visible_columns(const std::string& s);
std::string clip_ansi(const std::string& s, int max_cols);
std::string compose_screen(const std::vector<std::string>& lines,
                           int rows, int cols,
                           int cursor_row = -1, int cursor_col = -1,
                           const std::string& bg_mode = "mica");

std::string highlight_kestral_line(const std::string& line);
std::vector<std::string> discover_kes_files(const std::string& root_dir = ".",
                                            std::size_t limit = 40);
const std::vector<NanaTemplate>& nana_templates();

struct NanaBuffer {
    std::vector<std::string> lines;
    std::string filepath;
    int row = 0;
    int col = 0;
    int scroll_y = 0;
    int scroll_x = 0;
    bool modified = false;
    bool has_cut = false;
    std::string cut_buffer;
    std::string status;

    NanaBuffer() : lines{""} {}
    static NanaBuffer from_file(const std::string& path);
    static NanaBuffer from_template(const NanaTemplate& tpl);

    std::string text() const;
    void clamp_col();
    void insert_char(char ch);
    void insert_tab();
    void insert_newline();
    void backspace();
    void del();
    void move_left();
    void move_right();
    void move_up();
    void move_down();
    void move_home();
    void move_end();
    void cut_line();
    void paste_line();
    std::pair<bool, std::string> check_syntax() const;
    std::string save(const std::string& target = "");
    std::string render_frame(int rows, int cols,
                             const std::string& bg_mode = "mica");
};

std::string render_welcome_menu(int selected, int rows, int cols,
                                bool has_active_buffer = false,
                                const std::string& bg_mode = "mica");
std::string render_template_picker(int selected, int rows, int cols,
                                   const std::string& bg_mode = "mica");
std::string render_file_picker(const std::vector<std::string>& files,
                               int selected, int rows, int cols,
                               const std::string& root_dir = ".",
                               const std::string& bg_mode = "mica");

std::string prompt_bar(const std::string& label, const std::string& def = "",
                       NanaBuffer* buf = nullptr, const std::string& bg_mode = "mica",
                       bool* canceled = nullptr);
void show_overlay(const std::string& title, const std::string& body,
                  const std::string& bg_mode = "mica");
void run_interactive_code(const std::string& code, const std::string& title,
                          const std::vector<std::string>& cli_args,
                          const std::string& base_dir, RawTtyGuard& tty);

int run_nana(const std::string& filepath = "", const std::string& bg_override = "");

} // namespace kestral
