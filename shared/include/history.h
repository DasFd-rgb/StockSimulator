/** history.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef HISTORY_H
#define HISTORY_H
#include "gameData.h"
/* 체결 또는 폐지 기록을 최근 256건 배열에 추가합니다. 가득 차면 가장 오래된 한 건을 memmove로 제거합니다. 미체결 등록이나 취소는 거래 체결 기록에 넣지 않습니다. */
void history_trade(Game *game, const TradeRecord *record);
/* 현재 회차·현금·총자산을 최근 128건 배열에 추가합니다. 매매 후와 가격 갱신 완료 후 호출합니다. 총자산 계산 한도 초과 시 표시용 상한값을 기록합니다. */
void history_asset(Game *game);
/* 회사의 현재 가격을 최근 64개 가격 배열에 추가합니다. 최초 상장 가격도 포함합니다. 저장 상태에서는 마지막 이력과 현재가가 같아야 합니다. */
void history_price(Company *company);
/* 누적 손익·비용 계산이 지정한 금액 범위를 벗어나지 않도록 더합니다. 한도를 넘으면 경계값에 고정합니다. 거래 자체의 현금 변경은 별도 사전 검증을 거칩니다. */
int64_t bounded_add(int64_t a, int64_t b);
#endif
