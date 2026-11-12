/** [모듈 설명]
 * gameControl.c - 한 번의 정상 시장 갱신
 * 역할: 기존 시장·재무·가격·폐지·체결 순서를 재사용하여 안전하게 한 번에 반영합니다.
 * 관리 대상: 성공 시 Game 전체
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "gameControl.h"
#include "company.h"
#include "market.h"
#include "stockPrice.h"
#include "listing.h"
#include "order.h"
#include "portfolio.h"
#include "history.h"
/** [학습 설명]
 * 함수: game_tick
 * Game 복사본을 동적 메모리에 만들고 시장→재무→가격→폐지→지정가 체결→시장 기간 종료→자산 기록을 완료합니다. 전부 성공해야 원본에
 * 반영합니다. 실패하면 복사본을 해제하고 false를 반환합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - report: 이번 가격 갱신·체결·폐지 결과를 담는 출력 구조체.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool game_tick(Game *game, TickReport *report)
{
    Game *next;
    memset(report, 0, sizeof(*report));
    if (portfolio_bankrupt(game) || game->next_company_id > UINT64_MAX - COMPANY_COUNT - 1 ||
        game->tick_count == UINT64_MAX)
        return false;

    next = malloc(sizeof(*next));
    if (!next)
        return false;
    *next = *game;
    /* 반드시 지킬 순서: 시장 → 재무 → 전체 가격 → 폐지 정리 → 남은 주문 체결.
     * 폐지 가격에 도달한 주식을 먼저 팔아 손실을 피하는 틈을 막습니다. */
    market_begin(next, report);
    company_finance_update(next);
    ++next->tick_count;
    stock_prices_update(next, report);
    /* listing.c: order.c로 주문을 취소하고 company.c로 신규 회사 등록. */
    if (!listing_process(next, report))
    {
        free(next);
        return false;
    }
    /* order.c: 폐지 정리가 끝났으므로 살아 있는 종목만 체결합니다. */
    order_process(next, report);
    market_end(next, report);
    next->remaining_ms = TICK_MS;
    /* history.c: 체결까지 끝난 시점의 자산을 기록합니다. */
    history_asset(next);
    (void)snprintf(report->message, sizeof(report->message),
                   "상승 %u / 하락 %u / 보합 %u | 폐지 %u | 주문 체결 %u / 취소 %u%s",
                   report->rising, report->falling, report->sideways, report->delisted,
                   report->filled, report->failed_orders,
                   report->crash_started ? " | 폭락 시작"
                                         : (report->crash_ended ? " | 극단적 공포 해소" : ""));
    *game = *next;
    free(next);
    return true;
}
