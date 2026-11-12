/* market.h: 공포지수와 시장 상태. 100bp = 1%입니다. */
#ifndef MARKET_H
#define MARKET_H
#include "gameData.h"
MarketPhase market_phase_from_fear(int fear);
const char *market_phase_name(MarketPhase phase);
/* 목표와 기간 추첨 → 공포지수 이동 → 업종 영향 추첨 */
void market_begin(Game *game, TickReport *report);
/* 이번 회차를 소비합니다. 0이면 다음 갱신에서 새 목표를 추첨합니다. */
void market_end(Game *game, TickReport *report);
#endif
