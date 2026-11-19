/* main.c: 프로그램의 실행 순서를 여기서 직접 읽습니다.
 * 1. 실행 준비 → 2. 시간 갱신 → 3. 입력 → 4. 명령 처리 → 5. 화면 → 6. 저장/종료
 * 복잡한 거래 계산은 각 모듈 함수가 담당하며, 이 파일은 호출 순서를 담당합니다. */
#include "../shared/include/session.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "presentation.h"

int main(int argc, char **argv)
{
    Session *app;
    uint32_t shown_seconds = UINT32_MAX;
    int exit_code = 0;

    /* 허용하지 않는 실행 인수로 별도 저장 슬롯을 만들 수 없게 합니다. */
    if (argc != 1)
    {
        puts("Stock simulator V1. Run without arguments. h: in-game help.");
        return argc == 2 && !strcmp(argv[1], "--help") ? 0 : 1;
    }
    /* Game 안의 이력 배열이 크므로 스택 대신 배운 동적 메모리를 사용합니다. */
    app = malloc(sizeof(*app));
    if (!app)
        return 1;
    if (!session_open(app, 3000, 42))
    {
        session_close();
        free(app);
        return 1;
    }
    app->screen.title = "주식 모의투자 V1 시연판";
    app->screen.footer = "pause 정지 | resume 재개 | step 1회 진행 | h 도움말";
    app->screen.extra_help = "시연: pause / resume / step / event crash / event boom / event "
                             "delist ID / event fill 주문ID / reset";

    /* 입력을 기다리며 멈추지 않고, 반복마다 시간과 키보드를 확인합니다. */
    while (!app->quit)
    {
        int consumed;
        uint32_t seconds;
        session_advance_time(app);
        if (GetTickCount64() - app->last_checkpoint >= 5000)
        {
            (void)session_checkpoint(app);
            app->last_checkpoint = GetTickCount64();
        }

        /* Ctrl+C는 본 저장 대신 복구 파일을 남깁니다. q는 명령 함수에서 본 저장합니다. */
        if (session_stop_requested())
        {
            session_advance_time(app);
            if (session_checkpoint(app))
                break;
        }

        /* 한 번에 최대 64문자만 처리하여 붙여넣기 중에도 시간이 갱신됩니다. */
        for (consumed = 0; consumed < 64 && !app->quit; ++consumed)
        {
            int character = input_poll();
            if (character == -1)
                break; /* 아직 입력 없음 */
            if (character == -2 || character == 3)
            {
                session_advance_time(app);
                if (!session_checkpoint(app))
                    exit_code = 1;
                app->quit = true;
                break;
            }
            if (input_feed(&app->input, character))
            {
                /* Enter를 누른 시점의 가격을 먼저 확정하고 거래합니다. */
                session_advance_time(app);
                if (app->input.invalid)
                    (void)snprintf(app->notice, sizeof(app->notice),
                                   "입력이 너무 길거나 잘못되어 명령을 취소했습니다.");
                else
                {
                    /* 시연에서만 추가 명령을 먼저 확인합니다. 함수 주소 전달 없이 직접 호출합니다. */
                    if (!presentation_dispatch(app))
                        session_command(app);
                }
                input_clear(&app->input);
            }
            app->redraw = true;
        }

        /* 타이머는 1초마다, 입력과 거래 결과는 바뀌었을 때 출력합니다. */
        seconds = (app->game.remaining_ms + 999) / 1000;
        if (screen_interactive() && seconds != shown_seconds)
            app->redraw = true;
        if (app->redraw && !app->quit)
        {
            app->screen.paused = app->control.paused;
            screen_render(&app->game, &app->screen, app->notice, app->input.text, app->save_failed,
                          app->save_path);
            app->redraw = false;
            shown_seconds = seconds;
        }
        if (!app->quit)
            Sleep(20); /* CPU를 계속 점유하지 않도록 잠깐 양보 */
    }

    session_close();
    puts(exit_code ? "복구 기록 실패. 마지막 정상 상태를 확인하세요."
                   : "진행 상태를 보존하고 종료했습니다.");
    free(app);
    return exit_code;
}
