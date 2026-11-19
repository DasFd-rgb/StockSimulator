/* stockPrice.c: 공통 심리 + 업종 + 기업 재무 + 개별 난수로 등락률을 만듭니다.
 * 실제 시장의 30초 수익률을 복제하지 않는 게임용 시간 압축 모델입니다.
 * 모든 종목이 강제로 상승/하락하는 분기는 없습니다. 100bp = 1%. */
#include "stockPrice.h"
#include "random.h"
#include "history.h"

static int clamp(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}

/* game: 갱신할 게임 주소, report: 상승/하락/보합 개수를 받는 주소.
 * 모든 회사의 가격만 변경합니다. 폐지·주문 체결은 호출자가 다음에 수행합니다. */
void stock_prices_update(Game *game, TickReport *report)
{
    static const int range[] = {30, 80, 250};
    static const int beta[] = {90, 100, 120};
    /* 지수 50에서 방향 압력은 0. 구간 경계에서 갑자기 변화하지 않습니다. */
    int pressure = (50 - game->fear) + random_signed(game, -15, 15);
    size_t i;
    for (i = 0; i < COMPANY_COUNT; ++i)
    {
        Company *c = &game->companies[i];
        int spread = range[c->type];
        int finance = clamp(c->profit_bp / 600 - c->debt_percent / 120 + 1, -3, 3);
        int noise = random_signed(game, 0, spread) - random_signed(game, 0, spread);
        /* +x%, -x% 반복에서도 가격은 감소할 수 있어 과도한 변동 폭을 줄였습니다.
         * 재무 항은 시장 평균에 작은 성장 압력을 주지만 개별 수익을 보장하지 않습니다. */
        int change = finance + pressure * beta[c->type] / 100 + game->sector_bp[c->sector] + noise;
        if (c->type == STOCK_SPECULATIVE && random_range(game, 1, 100) <= 1)
        {
            int shock = random_signed(game, 500, 1500);
            change += random_range(game, 0, 1) ? shock : -shock;
        }
        change = clamp(change, -3000, 3000);
        c->last_change_bp = change;
        /* 정수 원 단위 반올림: 매번 버리면서 저가주가 불리해지는 편향을 줄입니다. */
        c->price = (c->price * (10000 + (int64_t)change) + 5000) / 10000;
        if (c->price > MAX_PRICE)
            c->price = MAX_PRICE;
        history_price(c);
        if (change > 0)
            ++report->rising;
        else if (change < 0)
            ++report->falling;
        else
            ++report->sideways;
    }
}
