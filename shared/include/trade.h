/** trade.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef TRADE_H
#define TRADE_H
#include "gameData.h"
/* 현재 가격으로 한 건을 전량 체결합니다. 잔액·예약·수량·계산 범위를 먼저 검사한 뒤 현금, 수량, 원가, 누적 비용·손익과 체결 기록을 바꿉니다. 일반 시장가는 order_id=0입니다. */
Result trade_execute(Game *game, uint64_t company_id, TradeSide side, int64_t quantity,
                     uint64_t order_id);
/* 폐지 직전 회사의 보유 주식을 0원에 소멸시키고 남은 매입 원가를 실현 손실로 기록합니다. 보유 수량과 원가를 모두 0으로 바꿉니다. 신규 회사 생성 전 호출합니다. */
void trade_delist(Game *game, size_t slot);
#endif
