/** listing.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef LISTING_H
#define LISTING_H
#include "gameData.h"
/* 먼저 폐지 대상 배열 위치를 모두 수집합니다. 해당 회사 주문 취소와 보유 자산 소멸을 전부 처리한 뒤 새 회사를 생성합니다. 이 순서로 폐지 대상이 신규 회사로 잘못 연결되지 않도록 합니다. */
bool listing_process(Game *game, TickReport *report);
#endif
