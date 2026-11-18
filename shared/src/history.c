/** [모듈 설명]
 * history.c - 최근 기록 보관
 * 역할: 배열이 가득 차면 오래된 자료를 하나 제거하고 새 자료를 끝에 넣습니다. 연결 리스트나 트리를 사용하지 않습니다.
 * 관리 대상: trades, assets, prices와 각 count
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "history.h"
#include "portfolio.h"
/** [학습 설명]
 * 함수: bounded_add
 * 누적 손익·비용 계산이 지정한 금액 범위를 벗어나지 않도록 더합니다. 한도를 넘으면 경계값에 고정합니다. 거래 자체의 현금 변경은 별도
 * 사전 검증을 거칩니다.
 * 매개변수:
 * - a: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * - b: a에 더할 손익 또는 비용 값. 음수도 가능합니다.
 * 반환: a+b를 -MAX_MONEY부터 MAX_MONEY 사이로 제한한 정수 금액.
 */
int64_t bounded_add(int64_t a, int64_t b)
{
    if (b > 0 && a > MAX_MONEY - b)
        return MAX_MONEY;
    if (b < 0 && a < -MAX_MONEY - b)
        return -MAX_MONEY;
    return a + b;
}
/** [학습 설명]
 * 함수: history_price
 * 회사의 현재 가격을 최근 64개 가격 배열에 추가합니다. 최초 상장 가격도 포함합니다. 저장 상태에서는 마지막 이력과 현재가가 같아야
 * 합니다.
 * 매개변수:
 * - company: 회사 구조체 주소. 가격·회사 이력을 읽거나 변경합니다.
 * 반환: 없음(void). 처리 결과는 이 함수가 수정하는 상태와 호출한 함수를 확인합니다.
 */
void history_price(Company *company)
{
    if (company->price_count == PRICE_HISTORY_MAX)
    {
        memmove(company->prices, company->prices + 1, (PRICE_HISTORY_MAX - 1) * sizeof(int64_t));
        --company->price_count;
    }
    company->prices[company->price_count++] = company->price;
}
/** [학습 설명]
 * 함수: history_trade
 * 체결 또는 폐지 기록을 최근 256건 배열에 추가합니다. 가득 차면 가장 오래된 한 건을 memmove로 제거합니다. 미체결 등록이나
 * 취소는 거래 체결 기록에 넣지 않습니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - record: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 없음(void). 처리 결과는 이 함수가 수정하는 상태와 호출한 함수를 확인합니다.
 */
void history_trade(Game *game, const TradeRecord *record)
{
    if (game->trade_count == TRADE_HISTORY_MAX)
    {
        memmove(game->trades, game->trades + 1, (TRADE_HISTORY_MAX - 1) * sizeof(TradeRecord));
        --game->trade_count;
    }
    game->trades[game->trade_count++] = *record;
}
/** [학습 설명]
 * 함수: history_asset
 * 현재 회차·현금·총자산을 최근 128건 배열에 추가합니다. 매매 후와 가격 갱신 완료 후 호출합니다. 총자산 계산 한도 초과 시 표시용
 * 상한값을 기록합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * 반환: 없음(void). 처리 결과는 이 함수가 수정하는 상태와 호출한 함수를 확인합니다.
 */
void history_asset(Game *game)
{
    AssetRecord record;
    record.tick = game->tick_count;
    record.cash = game->cash;
    (void)portfolio_total(game, &record.total);
    if (game->asset_count == ASSET_HISTORY_MAX)
    {
        memmove(game->assets, game->assets + 1, (ASSET_HISTORY_MAX - 1) * sizeof(AssetRecord));
        --game->asset_count;
    }
    game->assets[game->asset_count++] = record;
}

/* [관련 모듈] trade.c → history.c → saveLoad.c/screenOut.c
 * 보유 주식이 폐지될 때만 호출합니다. 고정 배열을 쓰므로 기록이 무한히 늘지 않습니다.
 * 오래된 상세가 빠져도 확인 전 전체 건수와 손실 합계는 남습니다. */
void history_delist_notice(Game *game, const TradeRecord *record)
{
    if (game->delist_notice_count == DELIST_NOTICE_MAX)
    {
        memmove(game->delist_notices, game->delist_notices + 1,
                (DELIST_NOTICE_MAX - 1) * sizeof(TradeRecord));
        --game->delist_notice_count;
    }
    game->delist_notices[game->delist_notice_count++] = *record;
    if (game->delist_notice_total < UINT64_MAX)
        ++game->delist_notice_total;
    game->delist_notice_loss = bounded_add(game->delist_notice_loss, -record->realized);
}
