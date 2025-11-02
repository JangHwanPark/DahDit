#include <string.h>
#include "morse.h"

typedef struct { const char* m; char ch; } MorseMap;

static const MorseMap MAP[] = {
    {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'},
    {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'},
    {"-.-", 'K'}, {".-..", 'L'}, {"--", 'M'}, {"-.", 'N'}, {"---", 'O'},
    {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
    {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'}, {"-.--", 'Y'},
    {"--..", 'Z'},
    {"-----",'0'},{".----",'1'},{"..---",'2'},{"...--",'3'},{"....-",'4'},
    {".....",'5'},{"-....",'6'},{"--...",'7'},{"---..",'8'},{"----.",'9'},
  };
static const int MAPN = (int)(sizeof(MAP)/sizeof(MAP[0]));

char morse_decode(const char* pattern) {
    for (int i = 0; i < MAPN; ++i) {
        if (strcmp(MAP[i].m, pattern) == 0) return MAP[i].ch;
    }
    return '?';
}
