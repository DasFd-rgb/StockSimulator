/** portfolio.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef PORTFOLIO_H
#define PORTFOLIO_H
#include "gameData.h"
/* 현금과 30개 보유 주식 평가액의 합을 구합니다. 계산 한도를 넘으면 total에 MAX_MONEY를 넣고 false를 반환하며 화면에는 최소 금액임을 표시합니다. */
bool portfolio_total(const Game *game, int64_t *total);
/* 실제 보유 현금과 보유 주식이 모두 없는지 검사합니다. 예약 현금도 현금에 포함되므로 available_cash가 0원이라는 이유로 파산시키지 않습니다. */
bool portfolio_bankrupt(const Game *game);
/* 모든 미체결 매수 주문의 reserved_cash를 합합니다. 예약금은 별도 계좌로 빼지 않고 실제 현금 안에서 사용만 제한합니다. */
int64_t reserved_cash(const Game *game);
/* 실제 현금에서 미체결 매수 예약금을 빼서 지금 새 주문에 쓸 수 있는 금액을 구합니다. */
int64_t available_cash(const Game *game);
/* 해당 회사 보유 수량에서 미체결 매도 주문의 예약 수량을 뺍니다. 이 값으로 중복 매도를 막습니다. */
int64_t available_shares(const Game *game, size_t slot);
/* 매도 수량에 해당하는 매입 원가를 정수 비례 배분합니다. 나눗셈의 몫과 나머지를 분리하여 큰 cost×quantity 곱셈의 오버플로를 피합니다. held는 0이 아니어야 합니다. */
int64_t proportional_cost(int64_t cost, int64_t quantity, int64_t held);
/* 금액에 basis point 단위 요율을 적용하여 원 단위로 올림합니다. 금액을 10,000의 몫과 나머지로 분해해 계산 범위를 보호합니다. */
int64_t rate_amount(int64_t amount, int basis_points);
/* 가격×수량과 매수 수수료의 합을 계산합니다. 가격·수량·계산 범위를 먼저 확인하고, 실패하면 false를 반환합니다. 지정가 예약금에도 같은 함수를 사용합니다. */
bool buy_cost(int64_t price, int64_t quantity, int64_t *total);
const char *result_message(Result result);
#endif
