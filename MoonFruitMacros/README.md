### Moon Fruit Macros
Moon Fruit Macros implements a C preprocessor but saves the state. Typically when debugging C macros, you can output the final text file that is fed to the parser, but this lacks all the middle steps (use gcc -E to generate this).

For instance if we have the following macros,

#define mac1 400
#define mac2 mac1 * 100

Then the compiler loses mac1 all together and we can only see the final expression. Moon Fruit Macros, saves the definitions but can output various steps of the preprocessor. We could output mac2 = mac1 * 100, or we could output 400 * 100.
