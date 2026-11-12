/** [모듈 설명]
 * portfolio.c - 자산 계산과 공통 거래 검증
 * 역할: 실제 현금과 예약 현금을 구분하고 수수료·비례 원가를 계산합니다. 화면도 같은 계산을 사용합니다.
 * 관리 대상: 대부분 읽기 전용 계산
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "portfolio.h"
/** [학습 설명]
 * 함수: portfolio_total
 * 현금과 30개 보유 주식 평가액의 합을 구합니다. 계산 한도를 넘으면 total에 MAX_MONEY를 넣고 false를 반환하며 화면에는
 * 최소 금액임을 표시합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - total: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool portfolio_total(const Game *game, int64_t *total)
{
    size_t i;
    int64_t sum = game->cash;
    for (i = 0; i < COMPANY_COUNT; ++i)
    {
        if (game->shares[i] > (MAX_MONEY - sum) / game->companies[i].price)
        {
            *total = MAX_MONEY;
            return false;
        }
        sum += game->shares[i] * game->companies[i].price;
    }
    *total = sum;
    return true;
}
/** [학습 설명]
 * 함수: portfolio_bankrupt
 * 실제 보유 현금과 보유 주식이 모두 없는지 검사합니다. 예약 현금도 현금에 포함되므로 available_cash가 0원이라는 이유로
 * 파산시키지 않습니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool portfolio_bankrupt(const Game *game)
{
    size_t i;
    if (game->cash)
        return false;
    for (i = 0; i < COMPANY_COUNT; ++i)
        if (game->shares[i])
            return false;
    return true;
}
/** [학습 설명]
 * 함수: reserved_cash
 * 모든 미체결 매수 주문의 reserved_cash를 합합니다. 예약금은 별도 계좌로 빼지 않고 실제 현금 안에서 사용만 제한합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
int64_t reserved_cash(const Game *game)
{
    uint32_t i;
    int64_t sum = 0;
    for (i = 0; i < game->order_count; ++i)
        sum += game->orders[i].reserved_cash;
    return sum;
}
/** [학습 설명]
 * 함수: available_cash
 * 실제 현금에서 미체결 매수 예약금을 빼서 지금 새 주문에 쓸 수 있는 금액을 구합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
int64_t available_cash(const Game *game)
{
    return game->cash - reserved_cash(game);
}
/** [학습 설명]
 * 함수: available_shares
 * 해당 회사 보유 수량에서 미체결 매도 주문의 예약 수량을 뺍니다. 이 값으로 중복 매도를 막습니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - slot: 회사 배열의 위치(0~29). 화면 정렬 순번이나 회사 고유 ID와 다릅니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
int64_t available_shares(const Game *game, size_t slot)
{
    uint32_t i;
    int64_t quantity = game->shares[slot];
    for (i = 0; i < game->order_count; ++i)
        if (game->orders[i].side == SIDE_SELL &&
            game->orders[i].company_id == game->companies[slot].id)
            quantity -= game->orders[i].quantity;
    return quantity;
}
/** [학습 설명]
 * 함수: rate_amount
 * 금액에 basis point 단위 요율을 적용하여 원 단위로 올림합니다. 금액을 10,000의 몫과 나머지로 분해해 계산 범위를
 * 보호합니다.
 * 매개변수:
 * - amount: 비율을 적용할 금액(원).
 * - basis_points: 요율 단위. 1bp=0.01%, 5bp=0.05%입니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
int64_t rate_amount(int64_t amount, int basis_points)
{

    /* 10,000으로 먼저 나눈 몫과 나머지로 계산하면 큰 곱셈을 피합니다.
     * 나머지에 9,999를 더한 정수 나눗셈은 원 미만 올림입니다. */
    return amount / 10000 * basis_points + ((amount % 10000) * basis_points + 9999) / 10000;
}
/** [학습 설명]
 * 함수: buy_cost
 * 가격×수량과 매수 수수료의 합을 계산합니다. 가격·수량·계산 범위를 먼저 확인하고, 실패하면 false를 반환합니다. 지정가 예약금에도
 * 같은 함수를 사용합니다.
 * 매개변수:
 * - price: 주당 금액(원). 정수로 다루어 소수점 금액 오차를 피합니다.
 * - quantity: 거래하거나 계산할 주식 수량(주). 양수와 최대 수량을 검사합니다.
 * - total: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool buy_cost(int64_t price, int64_t quantity, int64_t *total)
{
    int64_t gross, fee;
    if (price <= DELIST_PRICE || price > MAX_PRICE || quantity < 1 || quantity > MAX_SHARES ||
        quantity > MAX_MONEY / price)
        return false;
    gross = price * quantity;
    fee = rate_amount(gross, FEE_BP);
    if (gross > MAX_MONEY - fee)
        return false;
    *total = gross + fee;
    return true;
}
/** [학습 설명]
 * 함수: proportional_cost
 * 매도 수량에 해당하는 매입 원가를 정수 비례 배분합니다. 나눗셈의 몫과 나머지를 분리하여 큰 cost×quantity 곱셈의 오버플로를
 * 피합니다. held는 0이 아니어야 합니다.
 * 매개변수:
 * - cost: 보유 주식의 남은 총 매입 원가(원), 매수 비용 포함.
 * - quantity: 거래하거나 계산할 주식 수량(주). 양수와 최대 수량을 검사합니다.
 * - held: 현재 보유 수량. 비례 원가 계산 전에 0이 아님을 보장합니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
int64_t proportional_cost(int64_t cost, int64_t quantity, int64_t held)
{

    /* 예: 총 원가 100,050원 중 10주에서 4주를 팔면 원가 40,020원을 배분합니다.
     * 남은 원가는 그대로 보관하므로 전량 매도하면 잔액이 0이 됩니다. */
    return cost / held * quantity + (cost % held) * quantity / held;
}
/** [학습 설명]
 * 함수: result_message
 * Result 열거형 오류를 사용자가 읽을 수 있는 한국어 문장으로 바꿉니다. 기능 모듈이 직접 화면을 출력하는 대신 결과만 반환하게 하는
 * 연결부입니다.
 * 매개변수:
 * - result: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: Result 처리 결과. 실패 원인에 맞는 안내 문장으로 변환합니다.
 */
const char *result_message(Result result)
{
    switch (result)
    {
    case RESULT_OK:
        return "처리되었습니다.";
    case RESULT_BAD_INPUT:
        return "ID, 가격, 수량 또는 명령 형식이 올바르지 않습니다.";
    case RESULT_NO_COMPANY:
        return "현재 상장된 회사 ID가 아닙니다.";
    case RESULT_NO_CASH:
        return "예약 금액과 수수료를 제외한 사용 가능 현금이 부족합니다.";
    case RESULT_NO_SHARES:
        return "예약 수량을 제외한 매도 가능 주식이 부족합니다.";
    case RESULT_LIMIT:
        return "주문 개수 또는 계산 범위를 초과했습니다.";
    case RESULT_GAME_OVER:
        return "파산한 게임에서는 거래할 수 없습니다.";
    case RESULT_NO_ORDER:
        return "해당 미체결 주문이 없습니다.";
    default:
        return "처리 오류입니다.";
    }
}
