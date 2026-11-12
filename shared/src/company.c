/** [모듈 설명]
 * company.c - 회사·종목 기본 정보
 * 역할: 회사 고유 ID, 이름, 업종, 재무 정보와 최초 종목 구성을 관리합니다. 생성·갱신 책임을 이 파일에 모았습니다.
 * 관리 대상: Company, next_company_id, rng_state, 최초 Game
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "company.h"
#include "random.h"
#include "history.h"
static const char *const prefixes[] = {"가온", "누리", "다온", "라온", "마루", "바른", "새봄",
                                       "아람", "온빛", "이든", "초록", "하늘", "해솔", "한결",
                                       "별빛", "푸른", "새빛", "한울", "미래", "보람"};
static const char *const suffixes[] = {"전자", "제약",   "건설", "식품", "에너지",
                                       "로봇", "바이오", "운송", "유통", "화학"};
/* 뒷부분과 업종 일치: 기술0, 의료1, 산업2, 소비재3, 에너지4 */
static const int name_sectors[] = {0, 1, 2, 3, 4, 0, 1, 2, 3, 4};
/** [학습 설명]
 * 함수: sector_name
 * 업종 번호를 기술·의료·산업·소비재·에너지 문자열로 바꿉니다. 잘못된 번호는 오류 문자열로 표시합니다. 회사 이름의 제약·전자 등의
 * 뒷부분과 업종을 연결했습니다.
 * 매개변수:
 * - sector: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 호출자가 소유하거나 모듈이 보관한 문자열 주소. 반환 주소를 free하지 않습니다.
 */
const char *sector_name(int sector)
{
    static const char *const names[] = {"기술", "의료", "산업", "소비재", "에너지"};
    return sector >= 0 && sector < SECTOR_COUNT ? names[sector] : "오류";
}
/** [학습 설명]
 * 함수: company_name_valid
 * 저장된 회사 이름이 비어 있지 않고, 고정 크기 안에서 끝나며, UTF-8 한글 완성형으로만 이루어졌는지 확인합니다. 화면 제어 문자가
 * 저장 파일을 통해 들어오는 것도 차단합니다.
 * 매개변수:
 * - name: 검사·출력·변환할 회사 이름 문자열.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool company_name_valid(const char *name)
{
    size_t i = 0;
    if (!name[0])
        return false;
    while (i < NAME_SIZE && name[i])
    {
        const unsigned char *p = (const unsigned char *)name + i;
        uint32_t code;
        if (i + 2 >= NAME_SIZE || !p[1] || !p[2] || p[0] < 0xE0 || p[0] > 0xEF ||
            (p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80)
            return false;
        code = ((uint32_t)(p[0] & 15) << 12) | ((uint32_t)(p[1] & 63) << 6) | (p[2] & 63);
        if (code < 0xAC00 || code > 0xD7A3)
            return false;
        i += 3;
    }
    return i < NAME_SIZE;
}
/** [학습 설명]
 * 함수: company_find
 * 회사 고유 ID를 30개 배열에서 순서대로 찾습니다. 있으면 배열 위치 0~29, 없으면 -1을 반환합니다. 화면의 정렬 순서와 회사의
 * 실제 배열 위치를 혼동하지 않도록 합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - id: 찾을 고유 번호. 회사/주문 구분은 함수 이름과 위 설명을 따릅니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
int company_find(const Game *game, uint64_t id)
{
    int i;
    for (i = 0; i < COMPANY_COUNT; ++i)
        if (game->companies[i].id == id)
            return i;
    return -1;
}
/** [학습 설명]
 * 함수: name_used
 * 현재 상장된 30개 이름과 비교해 신규 이름 중복을 방지합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - name: 검사·출력·변환할 회사 이름 문자열.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
static bool name_used(const Game *game, const char *name)
{
    size_t i;
    for (i = 0; i < COMPANY_COUNT; ++i)
        if (game->companies[i].id && !strcmp(game->companies[i].name, name))
            return true;
    return false;
}
/** [학습 설명]
 * 함수: update_finance
 * 건강도를 바탕으로 이익률과 부채비율을 생성합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - company: 회사 구조체 주소. 가격·회사 이력을 읽거나 변경합니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
static void update_finance(Game *game, Company *company)
{
    int profit = (company->health - 50) * 35 + random_signed(game, -200, 200);
    int debt = 230 - company->health * 2 + random_signed(game, -15, 15);
    company->profit_bp = profit;
    company->debt_percent = debt < 0 ? 0 : debt;
}
/** [학습 설명]
 * 함수: company_replace
 * 지정한 배열 위치에 새로운 회사·ID·이름·재무 상태·첫 가격 이력을 생성합니다. 기존 보유 수량과 원가는 0이 되므로 폐지 손실을
 * 기록하는 trade_delist 이후에 호출합니다. ID 고갈이나 잘못된 인수는 false입니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - slot: 회사 배열의 위치(0~29). 화면 정렬 순번이나 회사 고유 ID와 다릅니다.
 * - type: 아래 처리에서 사용하는 입력 값입니다. 형식·단위는 선언과 계산식을 함께 확인합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool company_replace(Game *game, size_t slot, StockType type)
{
    Company next = {0};
    int unused[200], unused_count = 0, index, selected;
    static const uint32_t minimum[] = {3000000, 200000, 3000};
    static const uint32_t maximum[] = {5000000, 700000, 12000};
    static const int health_min[] = {75, 40, 10}, health_max[] = {95, 75, 50};
    if (slot >= COMPANY_COUNT || type < 0 || type > STOCK_SPECULATIVE ||
        game->next_company_id == 0 || game->next_company_id == UINT64_MAX)
        return false;
    /* 배열에 현재 쓰이지 않는 이름 번호를 모읍니다. 재추첨 무한 반복이 없습니다. */
    for (index = 0; index < 200; ++index)
    {
        (void)snprintf(next.name, sizeof(next.name), "%s%s", prefixes[index / 10],
                       suffixes[index % 10]);
        if (!name_used(game, next.name))
            unused[unused_count++] = index;
    }
    if (!unused_count)
        return false;
    selected = unused[random_range(game, 0, (uint32_t)unused_count - 1)];
    (void)snprintf(next.name, sizeof(next.name), "%s%s", prefixes[selected / 10],
                   suffixes[selected % 10]);
    next.id = game->next_company_id++;
    next.type = type;
    next.price = random_range(game, minimum[type], maximum[type]);
    next.sector = name_sectors[selected % 10];
    next.health = random_signed(game, health_min[type], health_max[type]);
    next.revenue_million = random_range(game, 100, 50000);
    update_finance(game, &next);
    history_price(&next);
    game->companies[slot] = next;
    game->shares[slot] = game->cost_basis[slot] = 0;
    return true;
}
/** [학습 설명]
 * 함수: company_finance_update
 * 다음 가격 갱신이 10회 단위에 해당하면 건강도와 매출을 조금 변경하고 이익률·부채비율을 다시 계산합니다. 나머지 갱신에서는 변경하지
 * 않습니다. 허용 범위로 제한합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
void company_finance_update(Game *game)
{
    size_t i;
    if ((game->tick_count + 1) % FINANCE_INTERVAL)
        return;
    for (i = 0; i < COMPANY_COUNT; ++i)
    {
        Company *company = &game->companies[i];
        static const int baseline[] = {85, 58, 30};
        int health = company->health + random_signed(game, -4, 4) +
                     (baseline[company->type] - company->health) / 12;
        company->health = health < 0 ? 0 : (health > 100 ? 100 : health);
        company->revenue_million += random_signed(game, -50, 50);
        if (company->revenue_million < 1)
            company->revenue_million = 1;
        if (company->revenue_million > 1000000)
            company->revenue_million = 1000000;
        update_finance(game, company);
    }
}
/** [학습 설명]
 * 함수: game_init
 * Game 전체를 0으로 초기화하고 현금 1천만 원, 난수 상태, 30개 회사, 최초 자산 기록을 만듭니다. seed가 0이면 고정된 0이
 * 아닌 상태를 사용합니다. 기존 게임을 초기화하는 호출이므로 이어 하기에는 사용하지 않습니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - seed: 새 게임의 최초 난수 상태. 진행 중 난수는 Game에 보관합니다.
 * 반환: 없음. 필요한 출력은 전달받은 구조체·버퍼에 기록합니다.
 */
void game_init(Game *game, uint64_t seed)
{
    StockType types[COMPANY_COUNT];
    size_t i;
    memset(game, 0, sizeof(*game));
    game->rng_state = seed ? seed : UINT64_C(0x9E3779B97F4A7C15);
    game->cash = INITIAL_CASH;
    game->next_company_id = game->next_order_id = 1;
    game->remaining_ms = TICK_MS;
    game->fear = 50;
    game->fear_target = 50;
    game->phase_ticks_left = random_range(game, 1, MARKET_DURATION_MAX);
    for (i = 0; i < COMPANY_COUNT; ++i)
        types[i] = i < 10 ? STOCK_BLUE_CHIP : (i < 25 ? STOCK_ORDINARY : STOCK_SPECULATIVE);
    for (i = COMPANY_COUNT - 1; i > 0; --i)
    {
        size_t j = random_range(game, 0, (uint32_t)i);
        StockType swap = types[i];
        types[i] = types[j];
        types[j] = swap;
    }
    for (i = 0; i < COMPANY_COUNT; ++i)
        (void)company_replace(game, i, types[i]);
    history_asset(game);
}
