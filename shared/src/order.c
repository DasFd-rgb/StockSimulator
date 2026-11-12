/** [모듈 설명]
 * order.c - 지정가 주문과 예약
 * 역할: 미체결 배열을 접수 순서로 관리하고 중복 사용을 막습니다. 체결 여부를 결정한 뒤 실제 자산 변경은 trade에 맡깁니다.
 * 관리 대상: orders, order_count, next_order_id
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "order.h"
#include "company.h"
#include "portfolio.h"
#include "trade.h"
#include "history.h"
/** [학습 설명]
 * 함수: remove_order
 * 중간 주문을 지운 뒤 남은 항목을 앞으로 당기고 마지막 슬롯을 비웁니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - index: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void remove_order(Game *game, uint32_t index)
{
    if (index + 1 < game->order_count)
        memmove(game->orders + index, game->orders + index + 1,
                (game->order_count - index - 1) * sizeof(Order));
    --game->order_count;
    memset(&game->orders[game->order_count], 0, sizeof(Order));
}
/** [학습 설명]
 * 함수: order_market
 * 시장가 주문을 trade_execute에 전달합니다. 성공한 경우 자산 변동 기록을 추가합니다. 미체결 배열에는 저장하지 않습니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - company_id: 회사 고유 ID. 상장폐지 후 신규 회사에는 다른 ID가 배정됩니다.
 * - side: 매수/매도 구분 열거형. 폐지 기록은 별도 구분을 사용합니다.
 * - quantity: 거래하거나 계산할 주식 수량(주). 양수와 최대 수량을 검사합니다.
 * 반환: Result 처리 결과. 실패 원인에 맞는 안내 문장으로 변환합니다.
 */
Result order_market(Game *game, uint64_t company_id, TradeSide side, int64_t quantity)
{
    Result result = trade_execute(game, company_id, side, quantity, 0);
    if (result == RESULT_OK)
        history_asset(game);
    return result;
}
/** [학습 설명]
 * 함수: order_process
 * 접수 순서대로 현재 가격과 지정가를 비교합니다. 체결 대상 주문 하나를 먼저 배열에서 빼 해당 예약만 해제하고 체결합니다. 나머지 주문의
 * 예약은 유지합니다. 수치 한도로 체결 실패한 주문은 취소합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - report: 이번 가격 갱신·체결·폐지 결과를 담는 출력 구조체.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
void order_process(Game *game, TickReport *report)
{
    uint32_t i = 0;
    while (i < game->order_count)
    {
        Order order = game->orders[i];
        int slot = company_find(game, order.company_id);
        bool eligible;
        Result result;
        if (slot < 0)
        {
            remove_order(game, i);
            ++report->failed_orders;
            continue;
        }
        eligible = order.side == SIDE_BUY ? game->companies[slot].price <= order.limit_price
                                          : game->companies[slot].price >= order.limit_price;
        if (!eligible)
        {
            ++i;
            continue;
        }

        /* 자기 주문의 예약만 먼저 해제합니다. 다른 주문의 예약은 유지해야
         * 한 현금·주식을 여러 주문이 중복 사용할 수 없습니다. */
        remove_order(game, i);
        result = trade_execute(game, order.company_id, order.side, order.quantity, order.id);
        if (result == RESULT_OK)
            ++report->filled;
        else
            ++report->failed_orders;
    }
}
/** [학습 설명]
 * 함수: order_limit
 * 인수·잔액·예약 수량·64개 주문 한도를 검사하고 고유 주문 ID를 부여합니다. 매수는 지정가 기준 수수료까지 예약합니다. 현재 가격으로
 * 이미 조건을 만족하면 즉시 체결을 시도합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - company_id: 회사 고유 ID. 상장폐지 후 신규 회사에는 다른 ID가 배정됩니다.
 * - side: 매수/매도 구분 열거형. 폐지 기록은 별도 구분을 사용합니다.
 * - quantity: 거래하거나 계산할 주식 수량(주). 양수와 최대 수량을 검사합니다.
 * - limit_price: 매수·매도 조건으로 지정한 주당 가격(원).
 * - order_id: 주문 고유 ID 또는 생성한 ID를 돌려줄 출력 주소.
 * 반환: Result 처리 결과. 실패 원인에 맞는 안내 문장으로 변환합니다.
 */
Result order_limit(Game *game, uint64_t company_id, TradeSide side, int64_t quantity,
                   int64_t limit_price, uint64_t *order_id)
{
    int slot = company_find(game, company_id);
    Order order = {0};
    TickReport report = {0};
    if (portfolio_bankrupt(game))
        return RESULT_GAME_OVER;
    if (slot < 0)
        return RESULT_NO_COMPANY;
    if ((side != SIDE_BUY && side != SIDE_SELL) || quantity < 1 || quantity > MAX_SHARES ||
        limit_price <= DELIST_PRICE || limit_price > MAX_PRICE)
        return RESULT_BAD_INPUT;
    if (game->order_count >= ORDER_MAX || game->next_order_id == UINT64_MAX)
        return RESULT_LIMIT;
    if (side == SIDE_BUY)
    {
        if (!buy_cost(limit_price, quantity, &order.reserved_cash))
            return RESULT_LIMIT;
        if (order.reserved_cash > available_cash(game))
            return RESULT_NO_CASH;
    }
    else if (quantity > available_shares(game, (size_t)slot))
        return RESULT_NO_SHARES;
    order.id = game->next_order_id++;
    order.company_id = company_id;
    order.created_tick = game->tick_count;
    order.side = side;
    order.quantity = quantity;
    order.limit_price = limit_price;
    game->orders[game->order_count++] = order;
    *order_id = order.id;
    order_process(game, &report);
    if (report.filled)
        history_asset(game);
    return report.failed_orders ? RESULT_LIMIT : RESULT_OK;
}
/** [학습 설명]
 * 함수: order_cancel
 * 주문 ID를 찾아 배열에서 제거합니다. 현금을 다시 더하지 않습니다. 예약금이 배열에서 빠져 available_cash가 늘어나는
 * 구조입니다. 없는 주문은 RESULT_NO_ORDER입니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - order_id: 주문 고유 ID 또는 생성한 ID를 돌려줄 출력 주소.
 * 반환: Result 처리 결과. 실패 원인에 맞는 안내 문장으로 변환합니다.
 */
Result order_cancel(Game *game, uint64_t order_id)
{
    uint32_t i;
    for (i = 0; i < game->order_count; ++i)
        if (game->orders[i].id == order_id)
        {
            remove_order(game, i);
            return RESULT_OK;
        }
    return RESULT_NO_ORDER;
}
/** [학습 설명]
 * 함수: order_cancel_company
 * 상장폐지된 회사 ID를 가진 모든 미체결 주문을 제거합니다. 배열을 당길 때 다음 항목을 건너뛰지 않도록 같은 위치를 다시 확인합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - company_id: 회사 고유 ID. 상장폐지 후 신규 회사에는 다른 ID가 배정됩니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
void order_cancel_company(Game *game, uint64_t company_id)
{
    uint32_t i = 0;
    while (i < game->order_count)
    {
        if (game->orders[i].company_id == company_id)
            remove_order(game, i);
        else
            ++i;
    }
}
