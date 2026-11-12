/** company.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef COMPANY_H
#define COMPANY_H
#include "gameData.h"
/* Game 전체를 0으로 초기화하고 현금 1천만 원, 난수 상태, 30개 회사, 최초 자산 기록을 만듭니다. seed가 0이면 고정된 0이 아닌 상태를 사용합니다. 기존 게임을 초기화하는 호출이므로 이어 하기에는 사용하지 않습니다. */
void game_init(Game *game, uint64_t seed);
/* 회사 고유 ID를 30개 배열에서 순서대로 찾습니다. 있으면 배열 위치 0~29, 없으면 -1을 반환합니다. 화면의 정렬 순서와 회사의 실제 배열 위치를 혼동하지 않도록 합니다. */
int company_find(const Game *game, uint64_t id);
/* 지정한 배열 위치에 새로운 회사·ID·이름·재무 상태·첫 가격 이력을 생성합니다. 기존 보유 수량과 원가는 0이 되므로 폐지 손실을 기록하는 trade_delist 이후에 호출합니다. ID 고갈이나 잘못된 인수는 false입니다. */
bool company_replace(Game *game, size_t slot, StockType type);
/* 저장된 회사 이름이 비어 있지 않고, 고정 크기 안에서 끝나며, UTF-8 한글 완성형으로만 이루어졌는지 확인합니다. 화면 제어 문자가 저장 파일을 통해 들어오는 것도 차단합니다. */
bool company_name_valid(const char *name);
const char *sector_name(int sector);
/* 다음 가격 갱신이 10회 단위에 해당하면 건강도와 매출을 조금 변경하고 이익률·부채비율을 다시 계산합니다. 나머지 갱신에서는 변경하지 않습니다. 허용 범위로 제한합니다. */
void company_finance_update(Game *game);
#endif
