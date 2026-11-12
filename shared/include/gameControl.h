/** gameControl.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef GAME_CONTROL_H
#define GAME_CONTROL_H
#include "gameData.h"
/* Game 복사본을 동적 메모리에 만들고 시장→재무→가격→폐지→지정가 체결→시장 기간 종료→자산 기록을 완료합니다. 전부 성공해야 원본에 반영합니다. 실패하면 복사본을 해제하고 false를 반환합니다. */
bool game_tick(Game *game, TickReport *report);
#endif
