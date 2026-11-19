/** [모듈 설명]
 * trade.c - 실제 체결과 손익 반영
 * 역할: 거래가 성립한 한 순간의 자산 변경을 담당합니다. 매수 원가에는 매수 수수료를 포함합니다.
 * 관리 대상: cash, shares, cost_basis, realized, fees_paid, taxes_paid, trades
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "trade.h"
#include "company.h"
#include "portfolio.h"
#include "history.h"
/** [학습 설명]
 * 함수: trade_execute
 * 현재 가격으로 한 건을 전량 체결합니다. 잔액·예약·수량·계산 범위를 먼저 검사한 뒤 현금, 수량, 원가, 누적 비용·손익과 체결 기록을
 * 바꿉니다. 일반 시장가는 order_id=0입니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - company_id: 회사 고유 ID. 상장폐지 후 신규 회사에는 다른 ID가 배정됩니다.
 * - side: 매수/매도 구분 열거형. 폐지 기록은 별도 구분을 사용합니다.
 * - quantity: 거래하거나 계산할 주식 수량(주). 양수와 최대 수량을 검사합니다.
 * - order_id: 주문 고유 ID 또는 생성한 ID를 돌려줄 출력 주소.
 * 반환: Result 처리 결과. 실패 원인에 맞는 안내 문장으로 변환합니다.
 */
Result trade_execute(Game *game, uint64_t company_id, TradeSide side, int64_t quantity,
                     uint64_t order_id)
{
    int slot = company_find(game, company_id);
    int64_t gross, fee, tax = 0, paid = 0, realized = 0;
    TradeRecord record = {0};
    if (portfolio_bankrupt(game))
        return RESULT_GAME_OVER;
    if (slot < 0)
        return RESULT_NO_COMPANY;
    if ((side != SIDE_BUY && side != SIDE_SELL) || quantity < 1 || quantity > MAX_SHARES)
        return RESULT_BAD_INPUT;
    if (quantity > MAX_MONEY / game->companies[slot].price)
        return RESULT_LIMIT;
    gross = game->companies[slot].price * quantity;
    fee = rate_amount(gross, FEE_BP);
    if (side == SIDE_BUY)
    {
        int64_t total;
        if (!buy_cost(game->companies[slot].price, quantity, &total))
            return RESULT_LIMIT;
        if (total > available_cash(game))
            return RESULT_NO_CASH;
        if (quantity > MAX_SHARES - game->shares[slot] ||
            total > MAX_MONEY - game->cost_basis[slot])
            return RESULT_LIMIT;
        /* 현금 지출과 매입 원가 모두에 매수 수수료를 포함합니다. */
        game->cash -= total;
        game->shares[slot] += quantity;
        game->cost_basis[slot] += total;
    }
    else
    {
        if (quantity > available_shares(game, (size_t)slot))
            return RESULT_NO_SHARES;
        tax = rate_amount(gross, SELL_TAX_BP);
        /* 매도 실수령액에서 해당 수량의 원가를 빼야 실현손익입니다. */
        paid = gross - fee - tax;
        if (paid > MAX_MONEY - game->cash)
            return RESULT_LIMIT;
        {
            int64_t removed_cost =
                proportional_cost(game->cost_basis[slot], quantity, game->shares[slot]);
            realized = paid - removed_cost;
            game->cost_basis[slot] -= removed_cost;
        }
        game->shares[slot] -= quantity;
        game->cash += paid;
        game->realized = bounded_add(game->realized, realized);
    }
    game->fees_paid = bounded_add(game->fees_paid, fee);
    game->taxes_paid = bounded_add(game->taxes_paid, tax);
    record.tick = game->tick_count;
    record.company_id = company_id;
    record.order_id = order_id;
    (void)snprintf(record.name, sizeof(record.name), "%s", game->companies[slot].name);
    record.side = side;
    record.quantity = quantity;
    record.price = game->companies[slot].price;
    record.fee = fee;
    record.tax = tax;
    record.realized = realized;
    history_trade(game, &record);
    return RESULT_OK;
}
/** [학습 설명]
 * 함수: trade_delist
 * 폐지 직전 회사의 보유 주식을 0원에 소멸시키고 남은 매입 원가를 실현 손실로 기록합니다. 보유 수량과 원가를 모두 0으로 바꿉니다.
 * 신규 회사 생성 전 호출합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - slot: 회사 배열의 위치(0~29). 화면 정렬 순번이나 회사 고유 ID와 다릅니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
void trade_delist(Game *game, size_t slot)
{
    if (game->shares[slot])
    {
        TradeRecord record = {0};
        record.tick = game->tick_count;
        record.company_id = game->companies[slot].id;
        (void)snprintf(record.name, sizeof(record.name), "%s", game->companies[slot].name);
        record.side = SIDE_DELIST;
        record.quantity = game->shares[slot];
        record.realized = -game->cost_basis[slot];
        game->realized = bounded_add(game->realized, record.realized);
        history_trade(game, &record);
    }
    game->shares[slot] = game->cost_basis[slot] = 0;
}
