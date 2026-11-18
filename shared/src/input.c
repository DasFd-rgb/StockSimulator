/** [모듈 설명]
 * input.c - 시간을 멈추지 않는 입력
 * 역할: scanf가 Enter를 기다리며 실행을 멈추는 방식 대신 문자 단위로 확인합니다. 입력 중에도 main의 반복문이 계속 실행됩니다.
 * 관리 대상: InputLine
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "input.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include <io.h>
/** [학습 설명]
 * 함수: input_poll
 * 콘솔 키보드나 리다이렉트된 입력에서 문자 하나만 확인합니다. 입력이 없으면 -1로 즉시 돌아오므로 타이머가 멈추지 않습니다. EOF는
 * -2, 콘솔의 비ASCII 문자는 별도 범위로 표시합니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
/* [windows.h 학습] GetConsoleMode가 성공하면 키보드 콘솔입니다.
 * _kbhit은 키 입력 유무, _getwch는 wchar_t 한 단위를 읽는 MSVC 함수입니다.
 * 파이프 입력에서는 PeekNamedPipe로 대기 바이트 수를 먼저 확인합니다.
 * ReadFile의 마지막에서 두 번째 인수 &count는 실제 읽은 바이트 수를 받습니다.
 * 이 함수의 -1은 아직 입력 없음, -2는 입력 끝 또는 입력 읽기 실패입니다. */
int input_poll(void)
{
    DWORD mode;
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (GetConsoleMode(input, &mode))
    {
        static bool extended;
        int key;
        if (!_kbhit())
            return -1;
        key = _getwch();
        if (extended)
        {
            extended = false;
            return -1;
        }
        if (key == 0 || key == 224)
        {
            extended = true;
            return -1;
        }
        return key >= 128 ? key + 0x10000 : key;
    }
    else
    {
        DWORD available = 0, count = 0;
        unsigned char character;
        if (GetFileType(input) == FILE_TYPE_PIPE)
        {
            if (!PeekNamedPipe(input, NULL, 0, NULL, &available, NULL))
                return -2;
            if (!available)
                return -1;
        }
        if (!ReadFile(input, &character, 1, &count, NULL) || !count)
            return -2;
        return character;
    }
}
/** [학습 설명]
 * 함수: input_clear
 * Enter 처리 후 입력 길이·내용·오류 표시를 초기화합니다. 가격이 갱신될 때는 호출하지 않으므로 입력 중인 내용이 보존됩니다.
 * 매개변수:
 * - line: 입력 중인 문자열·길이·잘못된 입력 여부를 담는 구조체.
 * 반환: 없음(void). 처리 결과는 이 함수가 수정하는 상태와 호출한 함수를 확인합니다.
 */
void input_clear(InputLine *line)
{
    memset(line, 0, sizeof(*line));
}
/** [학습 설명]
 * 함수: input_feed
 * 문자 하나를 입력 버퍼에 추가하고 Enter이면 true를 반환합니다. 콘솔 한글을 UTF-8로 바꾸고 Backspace는 한 글자
 * 단위로 지웁니다. 너무 긴 입력·제어 문자는 전체 명령을 무효화합니다.
 * 매개변수:
 * - line: 입력 중인 문자열·길이·잘못된 입력 여부를 담는 구조체.
 * - character: 키보드에서 받은 문자 또는 EOF/입력 없음 표시.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool input_feed(InputLine *line, int character)
{
    unsigned char encoded[3];
    size_t count = 0;
    if (character == '\n' || character == '\r')
        return true;
    if (character == 8 || character == 127)
    {
        if (line->length)
        {
            do
            {
                --line->length;
            } while (line->length && ((unsigned char)line->text[line->length] & 0xC0) == 0x80);
            line->text[line->length] = '\0';
        }
        return false;
    }
    if (character >= 0x10000)
    {
        unsigned int code = (unsigned int)(character - 0x10000);
        if (code >= 0xD800 && code <= 0xDFFF)
        {
            line->invalid = true;
            return false;
        }
        if (code < 0x800)
        {
            encoded[count++] = (unsigned char)(0xC0 | (code >> 6));
            encoded[count++] = (unsigned char)(0x80 | (code & 63));
        }
        else
        {
            encoded[count++] = (unsigned char)(0xE0 | (code >> 12));
            encoded[count++] = (unsigned char)(0x80 | ((code >> 6) & 63));
            encoded[count++] = (unsigned char)(0x80 | (code & 63));
        }
    }
    else if (character >= 32 && character <= 255)
        encoded[count++] = (unsigned char)character;
    else if (character == '\t')
        encoded[count++] = ' ';
    else
    {
        line->invalid = true;
        return false;
    }
    if (line->invalid)
        return false;
    if (line->length + count >= sizeof(line->text))
    {
        line->invalid = true;
        return false;
    }
    memcpy(line->text + line->length, encoded, count);
    line->length += count;
    line->text[line->length] = '\0';
    return false;
}
/** [학습 설명]
 * 함수: input_uint
 * 부호 없는 10진 정수 문자열을 검사하며 uint64_t 값으로 변환합니다. 음수·다른 문자·오버플로는 false입니다. 명령별 양수
 * 범위는 session.c의 positive가 추가로 검사합니다.
 * 매개변수:
 * - text: 검사하거나 표시할 문자열. 호출자가 내용과 종료 문자를 준비합니다.
 * - number: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool input_uint(const char *text, uint64_t *number)
{
    const unsigned char *p = (const unsigned char *)text;
    uint64_t value = 0;
    if (!*p)
        return false;
    for (; *p; ++p)
    {
        unsigned int digit;
        if (*p < '0' || *p > '9')
            return false;
        digit = (unsigned int)(*p - '0');
        if (value > (UINT64_MAX - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    *number = value;
    return true;
}
