### Moon Fruit Macros
 
In the C programming language, developers use "macros" which are essentially custom shortcuts to automatically expand short keywords to expressions or larger blocks of code before the program runs (like a smart search-and-replace).

#### The Problem
As projects grow, these shortcuts get layered inside one another. When something goes wrong, the standard tools only show you **final expanded result**. If there is a bug, you have no way of tracing the intermediate steps to see which shortcut the error.
 
For example, if you define these rules:
1. Replace `mac1` with `400`
2. Replace `mac2` with `mac1 * 100`
 
Standard tools will only show you the final result: `400 * 100`. The computer completely forgets about the name `mac1`, you to guess how that `400` got there.
 
#### The Solution
**Moon Fruit Macros** is a tool that runs these replacements but remembers every single step of the process. It allows you to the code at any point in its transformation (e.g., seeing `mac2 = mac1 * 100` before it becomes `400 * 100`), making complex rules effortless.

