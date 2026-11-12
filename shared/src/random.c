/** [모듈 설명]
 * random.c - 재현 가능한 난수
 * 역할: 가격 변화와 신규 회사, 공포지수의 무작위 선택을 한 난수 상태로 통합합니다. rand/srand를 다시 호출하지 않습니다.
 * 관리 대상: rng_state
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "random.h"
/** [학습 설명]
 * 함수: game_random
 * Game 안의 64비트 난수 상태를 xorshift 방식으로 갱신하고 의사난수를 반환합니다. 암호화 용도가 아닙니다. 상태를 저장하므로
 * 같은 상태에서 같은 미래가 재현됩니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
uint64_t game_random(Game *game)
{
    uint64_t state = game->rng_state;
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    game->rng_state = state;
    return state * UINT64_C(2685821657736338717);
}
/** [학습 설명]
 * 함수: random_range
 * low~high 양 끝을 포함하는 정수 난수를 반환합니다. 나머지 연산의 편향을 줄이는 재추첨을 수행합니다. 호출자는 low가 high
 * 이하가 되도록 전달합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - low: 난수·값 제한 범위의 최솟값(포함).
 * - high: 난수·값 제한 범위의 최댓값(포함).
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
uint32_t random_range(Game *game, uint32_t low, uint32_t high)
{
    uint64_t span = (uint64_t)high - low + 1, value;
    uint64_t threshold = (UINT64_C(0) - span) % span;
    do
    {
        value = game_random(game);
    } while (value < threshold);
    return low + (uint32_t)(value % span);
}
/** [학습 설명]
 * 함수: random_signed
 * 음수도 가능한 low~high 범위를 unsigned 난수 함수로 변환하여 사용합니다. 시장 변동과 재무 정보 변동에 사용합니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - low: 난수·값 제한 범위의 최솟값(포함).
 * - high: 난수·값 제한 범위의 최댓값(포함).
 * 반환: 위 설명의 계산값·위치·처리 결과. 특수값의 의미도 위 설명을 확인합니다.
 */
int random_signed(Game *game, int low, int high)
{
    return low + (int)random_range(game, 0, (uint32_t)(high - low));
}
