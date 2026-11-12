/* market.c: 시장 전체의 심리를 관리합니다.
 * 중립을 중심으로 탐욕/공포가 대칭으로 발생합니다. 극단보다 중립이 흔합니다.
 * 기간은 1~13회입니다. 기간 종료는 가격/지수 초기화를 의미하지 않습니다. */
#include "market.h"
#include "random.h"

static int clamp(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}

/* 표시와 가격 계산에서 같은 구간을 사용합니다. */
MarketPhase market_phase_from_fear(int fear)
{
    if (fear < 20)
        return PHASE_EXTREME_GREED;
    if (fear < 40)
        return PHASE_GREED;
    if (fear <= 60)
        return PHASE_NORMAL;
    if (fear <= 80)
        return PHASE_FEAR;
    return PHASE_CRASH;
}

const char *market_phase_name(MarketPhase phase)
{
    switch (phase)
    {
    case PHASE_EXTREME_GREED:
        return "극단적 탐욕";
    case PHASE_GREED:
        return "탐욕";
    case PHASE_FEAR:
        return "공포";
    case PHASE_CRASH:
        return "극단적 공포";
    default:
        return "중립";
    }
}

/* 장세가 바뀔 때는 인접한 상태로 이동합니다.
 * 중립으로 돌아갈 확률을 높여 양 극단이 계속 이어지는 것을 줄입니다.
 * 이전과 다른 상태를 선택하므로 한 표시 상태는 최대 13회만 유지됩니다. */
void market_begin(Game *game, TickReport *report)
{
    static const int low[] = {40, 81, 20, 0, 61};
    static const int high[] = {60, 100, 39, 19, 80};
    MarketPhase previous = game->phase;
    int movement, i;
    if (game->phase_ticks_left == 0)
    {
        switch (game->phase)
        {
        case PHASE_NORMAL:
            game->phase = random_range(game, 0, 1) ? PHASE_GREED : PHASE_FEAR;
            break;
        case PHASE_GREED:
            game->phase = random_range(game, 1, 100) <= 25 ? PHASE_EXTREME_GREED : PHASE_NORMAL;
            break;
        case PHASE_FEAR:
            game->phase = random_range(game, 1, 100) <= 25 ? PHASE_CRASH : PHASE_NORMAL;
            break;
        case PHASE_EXTREME_GREED:
            game->phase = PHASE_GREED;
            break;
        default:
            game->phase = PHASE_FEAR;
            break;
        }
        game->fear_target = random_signed(game, low[game->phase], high[game->phase]);
        game->phase_ticks_left = random_range(game, 1, MARKET_DURATION_MAX);
        /* 새 구간의 가장 가까운 경계로 옮깁니다. 주가는 초기화하지 않습니다. */
        game->fear = clamp(game->fear, low[game->phase], high[game->phase]);
    }
    movement = clamp((game->fear_target - game->fear) / 3, -5, 5);
    game->fear = clamp(game->fear + movement + random_signed(game, -2, 2), low[game->phase],
                       high[game->phase]);
    report->crash_started = previous != PHASE_CRASH && game->phase == PHASE_CRASH;
    report->crash_ended = previous == PHASE_CRASH && game->phase != PHASE_CRASH;
    for (i = 0; i < SECTOR_COUNT; ++i)
        game->sector_bp[i] = random_signed(game, -15, 15);
}

void market_end(Game *game, TickReport *report)
{
    (void)report;
    if (game->phase_ticks_left > 0)
        --game->phase_ticks_left;
}
