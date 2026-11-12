/** [모듈 설명]
 * stockQuery.c - 검색·정렬·관심 종목
 * 역할: 30개 원본 회사를 재배치하지 않고 위치 배열을 정렬합니다. 그래야 보유 수량이 다른 회사와 섞이지 않습니다.
 * 관리 대상: watched만 변경; 검색 결과는 임시 slots 배열
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "stockQuery.h"
#include "company.h"
/** [학습 설명]
 * 함수: compare
 * 선택한 정렬 기준의 두 회사 대소 관계를 반환합니다.
 * 매개변수:
 * - a: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * - b: 바이너리 버퍼 주소(현재 위치·끝·성공 여부 포함).
 * - sort: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
static int compare(const Company *a, const Company *b, SortMode sort)
{
    int64_t x, y;
    if (sort == SORT_NAME)
        return strcmp(a->name, b->name);
    if (sort == SORT_PRICE)
    {
        x = a->price;
        y = b->price;
    }
    else if (sort == SORT_CHANGE)
    {
        x = a->last_change_bp;
        y = b->last_change_bp;
    }
    else
        return a->id < b->id ? -1 : (a->id > b->id ? 1 : 0);
    return x < y ? -1 : (x > y ? 1 : 0);
}
/** [학습 설명]
 * 함수: stock_query
 * 회사 배열은 변경하지 않고 검색 결과의 배열 위치를 slots에 담습니다. 관심 여부와 이름·업종·ID 검색을 적용하고 삽입 정렬합니다.
 * 반환값은 결과 개수입니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - slots: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * - filter: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * - only_watched: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * - sort: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * - descending: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
size_t stock_query(const Game *game, int slots[COMPANY_COUNT], const char *filter,
                   bool only_watched, SortMode sort, bool descending)
{
    size_t count = 0, i, j;
    for (i = 0; i < COMPANY_COUNT; ++i)
    {
        const Company *c = &game->companies[i];
        char id[32];
        (void)snprintf(id, sizeof(id), "%" PRIu64, c->id);
        if (only_watched && !c->watched)
            continue;
        if (filter[0] && !strstr(c->name, filter) && !strstr(sector_name(c->sector), filter) &&
            !strstr(id, filter))
            continue;
        slots[count++] = (int)i;
    }

    for (i = 1; i < count; ++i)
    {
        int item = slots[i];
        j = i;
        while (j > 0)
        {
            int order = compare(&game->companies[slots[j - 1]], &game->companies[item], sort);
            if (descending)
                order = -order;
            if (order <= 0)
                break;
            slots[j] = slots[j - 1];
            --j;
        }
        slots[j] = item;
    }
    return count;
}
/** [학습 설명]
 * 함수: stock_watch
 * 현재 회사 ID를 찾아 watched 값을 반전합니다. 성공 여부를 반환합니다. 회사가 폐지되면 새 회사의 watched는
 * false이므로 관심 등록이 자동 승계되지 않습니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - id: 찾을 고유 번호. 회사/주문 구분은 함수 이름과 위 설명을 따릅니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool stock_watch(Game *game, uint64_t id)
{
    int slot = company_find(game, id);
    if (slot < 0)
        return false;
    game->companies[slot].watched = !game->companies[slot].watched;
    return true;
}
