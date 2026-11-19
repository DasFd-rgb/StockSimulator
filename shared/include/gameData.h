/** gameData.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef GAME_DATA_H
#define GAME_DATA_H
#include "common.h"
#include "gameConfig.h"
typedef enum
{
    STOCK_BLUE_CHIP,
    STOCK_ORDINARY,
    STOCK_SPECULATIVE
} StockType;
typedef enum
{
    PHASE_NORMAL,
    PHASE_CRASH,
    PHASE_GREED,
    PHASE_EXTREME_GREED,
    PHASE_FEAR
} MarketPhase;
typedef enum
{
    SIDE_BUY,
    SIDE_SELL,
    SIDE_DELIST
} TradeSide;
typedef struct
{
    uint64_t id;             /* 회사의 정체성. 배열 위치가 재사용돼도 이 번호는 새로 발급 */
    char name[NAME_SIZE];    /* UTF-8 가상 회사명. 현재 상장 회사끼리 중복 금지 */
    StockType type;          /* Hidden from the player's screens. */
    int sector;              /* 업종 번호: 기술/의료/산업/소비재/에너지 */
    int health;              /* 기업 건강도 0~100. 이익률·부채비율에 영향 */
    int profit_bp;           /* 영업이익률. 100bp가 1% */
    int debt_percent;        /* 부채비율(%). 높으면 재무 영향이 불리해짐 */
    int64_t revenue_million; /* 가상 매출액. 단위는 백만 원 */
    int64_t price;           /* 현재 1주 가격(원) */
    int32_t last_change_bp;
    bool watched;         /* 플레이어의 관심 등록 여부. 폐지 후 신규 회사에는 승계하지 않음 */
    uint32_t price_count; /* 가격 이력 배열에서 실제로 사용하는 개수 */
    int64_t prices[PRICE_HISTORY_MAX]; /* 오래된 순서. 가득 차면 첫 항목을 제거 */
} Company;
typedef struct
{
    uint64_t id;           /* 주문 고유 번호. 취소·체결 확인에 사용 */
    uint64_t company_id;   /* 주문 대상 회사의 고유 번호 */
    uint64_t created_tick; /* 접수한 갱신 회차. 배열의 접수 순서를 함께 유지 */
    TradeSide side;
    int64_t quantity;      /* 주문 수량(주). 분할 체결은 구현하지 않음 */
    int64_t limit_price;   /* 매수 상한 또는 매도 하한 가격(원) */
    int64_t reserved_cash; /* 매수 대금+수수료 예약액. 매도 주문은 0 */
} Order;
typedef struct
{
    uint64_t tick, company_id, order_id;
    char name[NAME_SIZE];
    TradeSide side;
    int64_t quantity; /* 체결 또는 폐지된 수량(주) */
    int64_t price;    /* 실제 체결가. 폐지 소멸 기록은 0원 */
    int64_t fee;      /* 해당 체결의 수수료(원) */
    int64_t tax;      /* 해당 체결의 매도세(원), 매수 시 0 */
    int64_t realized; /* 해당 매도/폐지로 확정된 손익(원) */
} TradeRecord;
typedef struct
{
    uint64_t tick;
    int64_t cash, total;
} AssetRecord;
typedef struct
{
    Company companies[COMPANY_COUNT];
    int64_t shares[COMPANY_COUNT];     /* 회사 배열의 같은 위치와 연결된 보유 수량 */
    int64_t cost_basis[COMPANY_COUNT]; /* 매수 수수료 포함 잔여 총 원가 */
    int64_t cash;                      /* 실제 현금. 예약 시에는 줄이지 않음 */
    int64_t realized;                  /* 확정된 매도·폐지 손익 누적 */
    int64_t fees_paid;                 /* 실제 체결 수수료 누적. 예약 수수료는 포함하지 않음 */
    int64_t taxes_paid;                /* 실제 납부한 매도세 누적 */
    uint64_t rng_state, next_company_id, next_order_id, tick_count;
    uint64_t save_generation; /* 복구 후보 중 가장 최근 상태를 고르는 저장 순번 */
    uint32_t remaining_ms;    /* 다음 가격 갱신까지 남은 시간. 종료 중에는 흐르지 않음 */
    int fear;                 /* 0 탐욕, 50 중립, 100 공포 */
    int fear_target;          /* 현재 장세에서 향하는 목표. 다음 장세까지 유지 */
    MarketPhase phase;
    uint32_t phase_ticks_left; /* 현재 목표를 유지할 남은 갱신 횟수: 0~12, 시작 직후 최대 13 */
    int sector_bp[SECTOR_COUNT];
    Order orders[ORDER_MAX];
    uint32_t order_count; /* 미체결 유효 항목 개수. 최대 ORDER_MAX */
    TradeRecord trades[TRADE_HISTORY_MAX];
    uint32_t trade_count; /* 최근 체결·폐지 기록 개수. 취소만 된 주문은 미포함 */
    AssetRecord assets[ASSET_HISTORY_MAX];
    uint32_t asset_count; /* 최근 자산 기록 개수. 초과하면 가장 오래된 기록 제거 */
} Game;
typedef enum
{
    RESULT_OK,
    RESULT_BAD_INPUT,
    RESULT_NO_COMPANY,
    RESULT_NO_CASH,
    RESULT_NO_SHARES,
    RESULT_LIMIT,
    RESULT_GAME_OVER,
    RESULT_NO_ORDER
} Result;
typedef struct
{
    uint32_t delisted, filled, failed_orders, rising, falling, sideways;
    bool crash_started, crash_ended;
    char message[256];
} TickReport;
typedef enum
{
    LOAD_OK,
    LOAD_BACKUP,
    LOAD_NOT_FOUND,
    LOAD_ERROR
} LoadResult;
typedef enum
{
    SORT_ID,
    SORT_PRICE,
    SORT_CHANGE,
    SORT_NAME
} SortMode;
typedef enum
{
    VIEW_MARKET,
    VIEW_DETAIL,
    VIEW_PORTFOLIO,
    VIEW_ORDERS,
    VIEW_TRADES,
    VIEW_ASSETS,
    VIEW_HELP
} ViewMode;
#endif
