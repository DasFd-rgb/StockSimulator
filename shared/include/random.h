/** random.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef GAME_RANDOM_H
#define GAME_RANDOM_H
#include "gameData.h"
/* Game 안의 64비트 난수 상태를 xorshift 방식으로 갱신하고 의사난수를 반환합니다. 암호화 용도가 아닙니다. 상태를 저장하므로 같은 상태에서 같은 미래가 재현됩니다. */
uint64_t game_random(Game *game);
/* low~high 양 끝을 포함하는 정수 난수를 반환합니다. 나머지 연산의 편향을 줄이는 재추첨을 수행합니다. 호출자는 low가 high 이하가 되도록 전달합니다. */
uint32_t random_range(Game *game, uint32_t low, uint32_t high);
/* 음수도 가능한 low~high 범위를 unsigned 난수 함수로 변환하여 사용합니다. 시장 변동과 재무 정보 변동에 사용합니다. */
int random_signed(Game *game, int low, int high);
#endif
