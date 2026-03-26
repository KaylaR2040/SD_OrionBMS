# Coding Standard Alignment

This project follows the [CMU C Coding Standard](https://users.ece.cmu.edu/~eno/coding/CCodingStandard.html).  
The key interpretations that now guide this codebase are summarised below so contributors
have a concise checklist alongside the upstream reference.

1. **File structure and headers**  
   - Every `.c` file begins with a short block comment describing its role.  
   - Includes are grouped: project headers first, then standard headers.  
   - File-scope `static` helpers and data are listed before public functions.

2. **Naming and scope**  
   - Constants use `SCREAMING_SNAKE_CASE`.  
   - Internal helpers are declared `static` to avoid leaking symbols.  
   - Each module encapsulates its own timing/context state (for example,
     `src/adc.c:24` tracks the next ADC run without leaking globals).

3. **Documentation and comments**  
   - Functions carry one-line summaries that describe purpose, side effects,
     and noteworthy implementation details (see `src/spi.c:32`).  
   - Complex sequences (e.g., MAX17841B command transactions) include inline
     comments explaining timing and error handling.

4. **Defensive programming**  
   - All public functions validate pointers and lengths up-front and return
     early when arguments are invalid.  
   - Configuration routines (clock, peripheral init) log failures before
     invoking `Error_Handler()` so issues remain traceable (`src/main.c:25-44`).

5. **Cooperative scheduling**  
   - The `while(true)` loop in `src/main.c` dispatches `ADC_ServiceTask`,
     and `SPI_ServiceTask`, letting each module track its
     own timing state (no central scheduler struct).  
   - Task bodies are short and self-contained, satisfying the guideline that
     loop bodies should call functions rather than contain complex logic.

When adding new modules, follow the same pattern:

```
/*
 * ===========================================================================
 * File: foo.c
 * Description: ...
 * ===========================================================================
 */
```

Group related declarations, explicitly document error paths, and avoid
magic numbers by introducing `#define` constants near the top of the file.
For quick reference, keep this document open alongside the CMU standard.
