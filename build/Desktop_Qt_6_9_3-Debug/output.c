#include "/home/ihor/Desktop/MyCompilerProject/src_compiler/pl0_runtime.h"

int main(void)
{
    if (setjmp(_env) == 0) {
    /* Program: lin */
    int32_t a, d, e, x, k;

{
k=9491;
printf("%s\n", "A/2 + 8*(D+E-K(9491)");
printf("%s\n", "Enter A:");
_safe_input(&a, "a");
printf("%s\n", "Enter D:");
_safe_input(&d, "d");
printf("%s\n", "Enter E:");
_safe_input(&e, "e");
x=a/_check_zero(2)+8*(d+e-k);
printf("%s\n", "Res:");
printf("%d\n", (int32_t)(x));

}

        return 0;
    }
    else {
        return _handle_runtime_error();
    }
}
