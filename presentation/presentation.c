/** [모듈 설명]
 * presentation.c - 별도 시연 프로그램
 * 역할: 진행 속도와 사건 재현은 이 프로젝트가 소유합니다.
 * 관리 대상: 시연용 게임 상태
 * 읽는 순서: 같은 이름의 .h 선언 → 구조체 → 공개 함수 → 내부 보조 함수.
 */
#include "presentation.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../shared/include/company.h"
#include "../shared/include/gameControl.h"
#include "../shared/include/history.h"
#include "../shared/include/input.h"
#include "../shared/include/listing.h"
#include "../shared/include/order.h"

/* 기능: 발표 중 사건을 재현합니다. 여기에만 인위적 사건 설정을 둡니다.
 * 입력: 현재 게임, 분리된 명령 토큰, 시간 제어 상태.
 * 출력: message 안내와 changed(영속 데이터 변경 여부).
 * 반환: 처리한 명령이면 true. 알 수 없는 명령은 공통 명령 처리기로 돌립니다.
 * 안전: 큰 상태 복사본에서 처리하고 성공한 변경만 원본에 반영합니다. */
/** [학습 설명]
 * 함수: presentation_command
 * 별도 시연 프로젝트에만 있는 기능입니다. 정지·재개·한 단계 진행과 폭락·폐지·주문 체결 상황을 재현합니다. 큰 상태 복사본에 적용한
 * 성공 결과만 반영합니다. 실사용 프로젝트에서 이 파일을 참조하지 않습니다.
 * 매개변수:
 * - game: 현재 게임 상태의 주소. const이면 읽기만 합니다.
 * - tokens: 공백으로 나눈 명령어와 인수 문자열 배열.
 * - count: 현재 유효한 항목 개수. 배열 최대 크기와 구분합니다.
 * - control: 현재 실행 프로그램의 가격 간격과 일시정지 여부.
 * - changed: 실제 게임 데이터 변경 여부를 반환하는 출력 주소.
 * - message: 사용자에게 표시할 결과 안내 문자열의 출력 주소.
 * - size: 출력 문자열 공간의 바이트 수. 잘라 쓰기로 범위 초과를 방지합니다.
 * 반환: 위에서 설명한 성공/조건 충족이면 true, 그 외에는 false입니다.
 */
bool presentation_command(Game *game, char *const *tokens, size_t count, RunControl *control,
                          bool *changed, char *message, size_t size)
{
    Game *next;
    TickReport report = {0};
    uint64_t id = 0;
    bool okay = false;
    *changed = false;
    if (count == 1 && (!strcmp(tokens[0], "pause") || !strcmp(tokens[0], "resume")))
    {
        control->paused = !strcmp(tokens[0], "pause");
        (void)snprintf(message, size, "%s",
                       control->paused ? "시연을 일시정지했습니다."
                                       : "3초 간격으로 다시 진행합니다.");
        return true;
    }
    if (strcmp(tokens[0], "step") && strcmp(tokens[0], "event") && strcmp(tokens[0], "reset"))
        return false;
    next = malloc(sizeof(*next));
    if (!next)
    {
        (void)snprintf(message, size, "시연 상태를 준비할 메모리가 부족합니다.");
        return true;
    }
    *next = *game;
    if (count == 1 && !strcmp(tokens[0], "step"))
    {
        control->paused = true;
        okay = game_tick(next, &report);
        if (okay)
            (void)snprintf(message, size, "%s", report.message);
    }
    else if (count == 1 && !strcmp(tokens[0], "reset"))
    {
        uint64_t generation = game->save_generation;
        game_init(next, 42);
        next->save_generation =
            generation; /* 과거 복구 파일이 새 상태보다 최신으로 보이지 않게 유지 */
        control->paused = true;
        okay = true;
        (void)snprintf(message, size,
                       "같은 시연 저장 안에서 초기 상태로 되돌렸습니다. resume으로 진행하세요.");
    }
    else if (count == 2 && !strcmp(tokens[0], "event") &&
             (!strcmp(tokens[1], "crash") || !strcmp(tokens[1], "boom")))
    {
        bool crash = !strcmp(tokens[1], "crash");
        next->fear = next->fear_target = crash ? 95 : 5;
        next->phase = crash ? PHASE_CRASH : PHASE_EXTREME_GREED;
        next->phase_ticks_left = 4;
        okay = true;
        (void)snprintf(message, size,
                       "시장 상황 설정. pause 후 step으로 확률적 가격 변화를 보세요.");
    }
    else if (count == 3 && !strcmp(tokens[0], "event") && input_uint(tokens[2], &id) && id)
    {
        if (!strcmp(tokens[1], "delist"))
        {
            int slot = company_find(next, id);
            if (slot >= 0)
            {
                next->companies[slot].price = 1000;
                okay = listing_process(next, &report);
                if (okay)
                    history_asset(next);
                (void)snprintf(
                    message, size,
                    "회사 ID %" PRIu64 " 폐지: 주문 취소·주식 소멸·신규 상장을 처리했습니다.", id);
            }
        }
        else if (!strcmp(tokens[1], "fill"))
        {
            uint32_t i;
            for (i = 0; i < next->order_count; ++i)
            {
                const Order *order = &next->orders[i];
                if (order->id == id)
                {
                    int slot = company_find(next, order->company_id);
                    if (slot >= 0)
                    {
                        next->companies[slot].price = order->limit_price;
                        history_price(&next->companies[slot]);
                        order_process(next, &report);
                        history_asset(next);
                        okay = report.filled > 0;
                        (void)snprintf(message, size, "주문 조건 재현: 체결 %u건, 취소 %u건.",
                                       report.filled, report.failed_orders);
                    }
                    break;
                }
            }
        }
    }
    if (okay)
    {
        next->remaining_ms = control->interval_ms;
        *game = *next;
        *changed = true;
    }
    else
        (void)snprintf(message, size,
                       "명령·회사/주문 ID를 확인하세요. 파산한 상태의 step은 실행할 수 없습니다.");
    free(next);
    return true;
}

/* 시연 명령만 분리합니다. 공통 명령은 false를 받아 main에서 직접 처리합니다. */
bool presentation_dispatch(Session *app)
{
    char copy[sizeof(app->input.text)], *tokens[6], *token;
    size_t count = 0;
    bool changed = false, handled;
    memcpy(copy, app->input.text, sizeof(copy));
    token = strtok(copy, " \t");
    while (token && count < 6)
    {
        tokens[count++] = token;
        token = strtok(NULL, " \t");
    }
    if (!count)
        return false;
    handled = presentation_command(&app->game, tokens, count, &app->control, &changed, app->notice,
                                   sizeof(app->notice));
    if (handled)
    {
        /* step에 소비된 현실 시간을 다음 회차에 이중 반영하지 않습니다. */
        app->last_clock = GetTickCount64();
        if (changed)
            (void)session_checkpoint(app);
        app->redraw = true;
    }
    return handled;
}
