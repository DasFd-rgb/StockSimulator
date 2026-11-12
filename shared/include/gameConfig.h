/** gameConfig.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H
/* All rates are game rules, NOT current Korean legal or broker rates. */
#define COMPANY_COUNT 30
#define NAME_SIZE 64
#define SECTOR_COUNT 5
#define PRICE_HISTORY_MAX 64  /* 회사별 최근 가격 개수 */
#define ORDER_MAX 64          /* 동시에 유지할 미체결 주문 최대 개수 */
#define TRADE_HISTORY_MAX 256 /* 최근 체결·폐지 기록 개수 */
#define ASSET_HISTORY_MAX 128 /* 최근 자산 변화 기록 개수 */
#define TICK_MS UINT32_C(30000)
#define INITIAL_CASH INT64_C(10000000)
#define DELIST_PRICE INT64_C(1000)
#define MAX_PRICE INT64_C(1000000000000)
#define MAX_MONEY INT64_C(9000000000000000)
#define MAX_SHARES INT64_C(100000000)
#define FEE_BP 5            /* 매수·매도 수수료 각각 0.05%. 실제 제도가 아닌 게임용 */
#define SELL_TAX_BP 15      /* 매도세 0.15%. 금액은 각 비용별 원 단위 올림 */
#define FINANCE_INTERVAL 10 /* 재무 정보는 가격 갱신 10회마다 변경 */
#define SAVE_VERSION 4U
#define MARKET_DURATION_MAX 13 /* 장세 하나의 최대 갱신 횟수 */
#endif
