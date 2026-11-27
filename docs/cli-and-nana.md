# Kestral CLI, `nana` Editor & `kestral learn`

## Commands

| Command | Purpose |
| --- | --- |
| `kestral` / `kestral repl` | Interactive multiline REPL with startup banner |
| `kestral <file.kes> [args...]` | Execute a program with forwarded `args()` |
| `kestral -e "<code>"` | Run inline Kestral snippet |
| `kestral check <file.kes>` | Parse & syntax-check without executing |
| `kestral nana [file.kes] [--bg mica\|black]` | Launch the minimalist `nana` TUI studio & editor |
| `kestral learn [n \| --list \| --reset] [--bg mica\|black]` | Launch the 10-lesson interactive CLI course |
| `kestral banner` | Print the monochrome `KES` startup card |

## REPL Commands

Inside `kestral`:
- `.help` (`help`) — show REPL commands
- `.nana [file.kes]` (`nana`) — open `nana` editor
- `.learn [n]` (`learn`) — open `kestral learn` course
- `.vars` (`vars`) — inspect user-defined variables
- `.run <file.kes>` — execute a file inside the current REPL session
- `.reset` (`.clear`) — clear user variables while keeping all built-ins active
- `.quit` (`.exit`) — leave the REPL

## Surface Compatibility (Mica & Black)

Both `nana` and `kestral learn` use minimalist hairline chrome (no full-width solid inverted bars), explicit `\x1b[row;1H` positioning with `SIGWINCH` live resizing, and two user-selectable surfaces saved in `~/.kestral/nana.json`:
- **`mica`** *(default)* — leaves empty terminal cells uncolored (`\x1b[49m`) so transparent Windows 11 Mica, macOS Vibrancy, or Kitty background blur pass through cleanly.
- **`black`** — renders true pitch black (`#000000`) across every column and row.

Press `t` in any menu/map screen, pick the Surface toggle in `nana`, or pass `--bg black` / `--bg mica`.
