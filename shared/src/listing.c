/** [모듈 설명]
 * listing.c - 폐지와 신규 상장의 연결
 * 역할: 가격 계산과 거래 체결 사이에 폐지 대상을 먼저 처리합니다. 사용자에게 1,000원 이하 주식을 매도할 틈을 주지 않습니다.
 * 관리 대상: 폐지 대상 회사·주문·보유 자산, 신규 회사
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "listing.h"
#include "company.h"
#include "order.h"
#include "trade.h"
#include "random.h"
/** [학습 설명]
 * 함수: listing_process
 * 먼저 폐지 대상 배열 위치를 모두 수집합니다. 해당 회사 주문 취소와 보유 자산 소멸을 전부 처리한 뒤 새 회사를 생성합니다. 이 순서로
 * 폐지 대상이 신규 회사로 잘못 연결되지 않도록 합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - report: 이번 가격 갱신·체결·폐지 결과를 담는 출력 구조체.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool listing_process(Game *game, TickReport *report)
{
    int slots[COMPANY_COUNT];
    size_t count = 0, i;
    for (i = 0; i < COMPANY_COUNT; ++i)
        if (game->companies[i].price <= DELIST_PRICE)
            slots[count++] = (int)i;
    for (i = 0; i < count; ++i)
    {
        size_t slot = (size_t)slots[i];
        order_cancel_company(game, game->companies[slot].id);
        trade_delist(game, slot);
    }
    for (i = 0; i < count; ++i)
    {
        StockType type = (StockType)random_range(game, 0, 2);
        if (!company_replace(game, (size_t)slots[i], type))
            return false;
    }
    report->delisted = (uint32_t)count;
    return true;
}
