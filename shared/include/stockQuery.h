/** stockQuery.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef STOCK_QUERY_H
#define STOCK_QUERY_H
#include "gameData.h"
/* 회사 배열은 변경하지 않고 검색 결과의 배열 위치를 slots에 담습니다. 관심 여부와 이름·업종·ID 검색을 적용하고 삽입 정렬합니다. 반환값은 결과 개수입니다. */
size_t stock_query(const Game *game, int slots[COMPANY_COUNT], const char *filter,
                   bool only_watched, SortMode sort, bool descending);
/* 현재 회사 ID를 찾아 watched 값을 반전합니다. 성공 여부를 반환합니다. 회사가 폐지되면 새 회사의 watched는 false이므로 관심 등록이 자동 승계되지 않습니다. */
bool stock_watch(Game *game, uint64_t id);
#endif
