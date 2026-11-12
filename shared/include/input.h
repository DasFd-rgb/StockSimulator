/** input.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef INPUT_H
#define INPUT_H
#include "gameData.h"
typedef struct
{
    char text[192];
    size_t length;
    bool invalid;
} InputLine;
/* 콘솔 키보드나 리다이렉트된 입력에서 문자 하나만 확인합니다. 입력이 없으면 -1로 즉시 돌아오므로 타이머가 멈추지 않습니다. EOF는 -2, 콘솔의 비ASCII 문자는 별도 범위로 표시합니다. */
int input_poll(void);
/* Returns true when an entire line is ready. Invalid lines never execute. */
/* 문자 하나를 입력 버퍼에 추가하고 Enter이면 true를 반환합니다. 콘솔 한글을 UTF-8로 바꾸고 Backspace는 한 글자 단위로 지웁니다. 너무 긴 입력·제어 문자는 전체 명령을 무효화합니다. */
bool input_feed(InputLine *line, int character);
/* Enter 처리 후 입력 길이·내용·오류 표시를 초기화합니다. 가격이 갱신될 때는 호출하지 않으므로 입력 중인 내용이 보존됩니다. */
void input_clear(InputLine *line);
/* 부호 없는 10진 정수 문자열을 검사하며 uint64_t 값으로 변환합니다. 음수·다른 문자·오버플로는 false입니다. 명령별 양수 범위는 session.c의 positive가 추가로 검사합니다. */
bool input_uint(const char *text, uint64_t *number);
#endif
