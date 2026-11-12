/** order.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef ORDER_H
#define ORDER_H
#include "gameData.h"
/* 시장가 주문을 trade_execute에 전달합니다. 성공한 경우 자산 변동 기록을 추가합니다. 미체결 배열에는 저장하지 않습니다. */
Result order_market(Game *game, uint64_t company_id, TradeSide side, int64_t quantity);
/* 인수·잔액·예약 수량·64개 주문 한도를 검사하고 고유 주문 ID를 부여합니다. 매수는 지정가 기준 수수료까지 예약합니다. 현재 가격으로 이미 조건을 만족하면 즉시 체결을 시도합니다. */
Result order_limit(Game *game, uint64_t company_id, TradeSide side, int64_t quantity,
                   int64_t limit_price, uint64_t *order_id);
/* 주문 ID를 찾아 배열에서 제거합니다. 현금을 다시 더하지 않습니다. 예약금이 배열에서 빠져 available_cash가 늘어나는 구조입니다. 없는 주문은 RESULT_NO_ORDER입니다. */
Result order_cancel(Game *game, uint64_t order_id);
/* 상장폐지된 회사 ID를 가진 모든 미체결 주문을 제거합니다. 배열을 당길 때 다음 항목을 건너뛰지 않도록 같은 위치를 다시 확인합니다. */
void order_cancel_company(Game *game, uint64_t company_id);
/* 접수 순서대로 현재 가격과 지정가를 비교합니다. 체결 대상 주문 하나를 먼저 배열에서 빼 해당 예약만 해제하고 체결합니다. 나머지 주문의 예약은 유지합니다. 수치 한도로 체결 실패한 주문은 취소합니다. */
void order_process(Game *game, TickReport *report);
#endif
