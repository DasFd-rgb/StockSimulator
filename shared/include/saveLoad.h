/** saveLoad.h: 다른 모듈이 사용하는 자료형·함수 선언입니다. 구현과 자세한 학습 설명은 .c에 있습니다. */
#ifndef SAVE_LOAD_H
#define SAVE_LOAD_H
#include "gameData.h"
/* 저장 전과 불러온 뒤 전체 상태의 범위와 관계를 검사합니다. 고유 ID·이름 중복, 가격 이력, 수량·원가, 예약금 합, 예약 매도 수량, 기록 순서를 검사합니다. 오류 문장을 선택적으로 반환합니다. */
bool game_validate(const Game *game, char *error, size_t size);
/* 내부 전용: 본 저장을 바꾸지 않고 최대 2개의 복구 파일을 번갈아 갱신 */
/* 게임 상태를 검증하고 저장 순번을 하나 높인 복사본을 두 고정 복구 파일 중 한 곳에 기록합니다. 본 저장은 변경하지 않습니다. 디스크 반영까지 성공한 경우에만 메모리의 순번을 갱신합니다. */
bool storage_checkpoint(const char *path, Game *game, char *error, size_t size);
/* q 종료 전용: 복구 상태를 먼저 보호한 뒤 본 저장을 갱신 */
/* q 종료에만 호출합니다. 최신 복구 파일을 먼저 완성한 뒤 같은 상태를 본 저장에 기록합니다. 본 저장 쓰기가 중단되어도 복구 상태가 남습니다. 별도 파일을 더 만들지 않습니다. */
bool storage_finish(const char *path, Game *game, char *error, size_t size);
/* 본 저장과 두 복구 파일의 길이·버전·검사값·상태를 검증하고 순번이 가장 큰 유효 후보를 자동 복원합니다. 사용자가 과거 상태를 고르는 기능은 없습니다. 셋 모두 손상됐다면 기존 메모리를 바꾸지 않고 오류를 반환합니다. */
LoadResult storage_load(const char *path, Game *game, char *error, size_t size);
#endif
