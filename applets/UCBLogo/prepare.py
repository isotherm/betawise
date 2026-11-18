"""Stage the pinned upstream sources with the small OS3K profile."""
from pathlib import Path
import re
import sys
import subprocess

upstream, dest = map(Path, sys.argv[1:])
revision = "a0215433f07384e4c42fe247525867f11e6f32f7"
try:
    head = subprocess.check_output(
        ["git", "-C", str(upstream), "rev-parse", "HEAD"], text=True
    ).strip()
except subprocess.CalledProcessError:
    raise SystemExit("Initialize the pinned UCBLogo submodule first. See README.md.")
if head != revision:
    raise SystemExit(f"UCBLogo checkout must be at {revision}, found {head}")
dest.mkdir(exist_ok=True)
commands = set("""
fd forward bk back rt right lt left repeat repcount cs clearscreen
pu penup pd pendown home print st showturtle ht hideturtle to ct cleartext er erase
""".split())
for path in upstream.iterdir():
    if path.suffix not in (".c", ".h"):
        continue
    source = path.read_text(encoding="latin1")
    if path.name == "logo.h":
        source = source.replace("#define SEG_SIZE        16000", "#define SEG_SIZE        128")
        source = source.replace("#define MAX_PHYS_LINE   5000", "#define MAX_PHYS_LINE   1024")
    elif path.name == "init.c":
        # Keep the evaluator and intern table initialization intact.
        source = re.sub(r'^([ \t]+)\{"([^"]+)".*?\},[^\n]*$',
                        lambda m: m[0] if m[2] in commands else "",
                        source, flags=re.M)
        start = source.index('    snprintf(linebuf, 256, "%s%sMessages"')
        end = source.index("\n#define wd_copy", start)
        source = source[:start] + """    char *message = (char *)neo_messages;
    for (i = 0; i < MAX_MESSAGE + NUM_WORDS; ++i) {
        message_texts[i] = message;
        message += strlen(message) + 1;
    }
""" + source[end:]
        source = '#include "messages.h"\n' + source
    elif path.name == "math.c":
        source = re.sub(r"^#define isdigit\([^\n]+\n", "", source, flags=re.M)
    elif path.name == "eval.c":
        source = "extern int neo_tick(void);\n" + source
        source = source.replace("val = (*getprimfun(proc))(argl);",
                                "val = neo_tick() ? (*getprimfun(proc))(argl) : UNBOUND;")
        source = source.replace("if (repcount == 0) {", "if (repcount == 0 || !neo_tick()) {")
    elif path.name == "graphics.c":
        source = "extern int neo_tick(void);\n" + source
        source = source.replace("wraploop:\n", "wraploop:\n    if (!neo_tick()) return;\n", 1)
        # The framebuffer is the drawing history. Disable replay and the
        # desktop XOR cursor, which would otherwise modify pen state.
        source = source.replace("BOOLEAN refresh_p = TRUE;", "BOOLEAN refresh_p = FALSE;")
        start = source.index("void draw_turtle(void) {")
        end = source.index("\n}", start) + 2
        source = source[:start] + "void draw_turtle(void) {}" + source[end:]
        source += """
/* Toggle the LCD cursor without changing Logo's pen or drawing history. */
extern void neo_cursor_edge(int, int, int, int);

void neo_toggle_turtle(void)
{
    if (!user_turtle_shown)
        return;
    /* Use the release's built-in triangle geometry at LCD scale. */
    FLONUM angle = (90.0 - turtle_heading) * degrad;
    FLONUM dx = sin(angle) * turtle_half_bottom * 0.55;
    FLONUM dy = cos(angle) * turtle_half_bottom * 0.55;
    FLONUM nose = turtle_side * 0.55;
    int left = screen_x_center + g_round(turtle_x - dx);
    int left_y = screen_y_center - g_round(turtle_y + dy);
    int right = screen_x_center + g_round(turtle_x + dx);
    int right_y = screen_y_center - g_round(turtle_y - dy);
    int tip = screen_x_center + g_round(turtle_x + cos(angle) * nose);
    int tip_y = screen_y_center - g_round(turtle_y + sin(angle) * nose);
    neo_cursor_edge(right, right_y, left, left_y);
    neo_cursor_edge(left, left_y, tip, tip_y);
    neo_cursor_edge(tip, tip_y, right, right_y);
}
"""

    elif path.name == "nographics.h":
        source = source.replace("#define prepare_to_draw_turtle nop();", "")
        source = source.replace("#define done_drawing_turtle nop();", "")
        source = """void neo_move(int, int);
void neo_line(int, int, int, int);
void neo_clear(void);
""" + source
        for old, new in {
            "#define GR_SIZE 1": "#define GR_SIZE 8",
            "#define screen_right 100": "#define screen_right 131",
            "#define screen_top 1": "#define screen_top 0",
            "#define screen_bottom 100": "#define screen_bottom 63",
            "#define clear_screen nop()": "#define clear_screen neo_clear()",
            "#define line_to(x,y) nop()": "#define line_to(x,y) neo_line(x,y,pen_vis,drawing_turtle)",
            "#define move_to(x,y) nop()": "#define move_to(x,y) neo_move(x,y)",
        }.items():
            assert old in source, old
            source = source.replace(old, new)
    source = re.sub(r"\b(fgets|fputc|fputs|putc)\b", r"neo_\1", source)
    (dest / path.name).write_text(source, encoding="latin1")
(dest / "config.h").write_bytes(Path("port_config.h").read_bytes())
message_file = (upstream / "logolib/Messages").read_bytes()
message_data = b"\0".join(
    line for line in message_file.splitlines() if not line.startswith(b";")
)
(dest / "messages.h").write_text(
    "static const unsigned char neo_messages[] = {\n" +
    "\n".join(", ".join(str(value) for value in message_data[i:i + 16]) + ","
              for i in range(0, len(message_data), 16)) + "\n0 };\n"
)
