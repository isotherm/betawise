#include "os3k.h"
#include <stddef.h>
_Static_assert(sizeof(MarkupResult_t) == 6, "native result size");
_Static_assert(offsetof(MarkupResult_t, offset) == 2, "native result offset");
_Static_assert(sizeof(MarkupElement_t) == 16, "element size");
#define OUT ((volatile uint32_t*)0x73c00)
static int16_t accept(const char* path, const char* name, int8_t depth, int8_t index)
{
    OUT[1] = depth; OUT[2] = index; OUT[3] = path[0]; OUT[4] = name[0];
    return 1;
}
void typed_probe(void)
{
    const char input[] = "<root a=\"1\"><child name=\"Joe\">A&amp;B</child></root>";
    MarkupElement_t nodes[4], children[2];
    int16_t n = -1; int8_t count = -1; char buffer[32]; uint32_t errors = 0;
    MarkupResult_t r = MarkupParse(input, input + sizeof(input) - 1, nodes, 4, accept);
    if (r.status || r.offset || r.reserved || OUT[1] != 2 || OUT[2] != 1 || OUT[3] != '/' || OUT[4] != 'c') ++errors;
    r = MarkupGetAttribute(nodes, "a", buffer, sizeof(buffer), &n);
    if (r.status || r.offset || r.reserved || n != 1 || buffer[0] != '1') ++errors;
    r = MarkupGetText(nodes + 1, buffer, sizeof(buffer), &n);
    if (r.status || r.offset || r.reserved || n != 3 || buffer[0] != 'A' || buffer[1] != '&' || buffer[2] != 'B' || buffer[3]) ++errors;
    r = MarkupFindElements(nodes, "/child", children, 2, &count);
    if (r.status || r.offset || r.reserved || count != 1) ++errors;
    r = MarkupGetChildAttribute(nodes, "/child", "name", buffer, &n);
    if (r.status || r.offset || r.reserved || n != 3 || buffer[0] != 'J') ++errors;
    r = MarkupGetChildText(nodes, "/child", buffer, sizeof(buffer), &n);
    if (r.status || r.offset || r.reserved || n != 3 || buffer[1] != '&') ++errors;
    r = MarkupGetText(nodes + 1, buffer, 2, &n);
    if (r.status != MARKUP_OUTPUT_FULL || r.reserved || n != 1 || buffer[0] != 'A' || buffer[1]) ++errors;
    OUT[0] = errors ? errors : 0x70617373;
}
