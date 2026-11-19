/** stockPrice.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef STOCK_PRICE_H
#define STOCK_PRICE_H
#include "gameData.h"
/* 모든 기존 종목 가격을 갱신합니다. 회사 재무 정보와 업종, 종목별 난수, 탐욕·공포 공통 압력을 반영합니다. 최종 ±30% 제한 후 원 단위 반올림하고 가격 이력을 남깁니다. 아직 폐지는 처리하지 않습니다. */
void stock_prices_update(Game *game, TickReport *report);
#endif
