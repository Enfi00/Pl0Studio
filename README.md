# Pl0Studio: PL/0 Custom Compiler

**Pl0Studio** is a custom compiler for a dialect of the educational programming language PL/0. The project combines classical translation theory (written in low-level C) with a modern graphical user interface (GUI) developed using the Qt 6 framework.

## Key Features

The compiler implements a full source code analysis cycle and consists of three key subsystems:

* **Lexical Analyzer (Scanner / Lexer):** Reads the source code and breaks it down into a sequence of atomic tokens (keywords, identifiers, numbers, special symbols). It ignores comments and whitespaces.
* **Syntax Analyzer (Parser):** Receives tokens from the lexer and checks them against the strict grammar of the PL/0 language. It is responsible for building the abstract syntax tree (AST) and generating clear syntax error messages.
* **Semantic Analyzer:** Verifies context-sensitive language rules. It checks if variables were declared before use, ensures type matching, and tracks variable scopes.

## Tech Stack

* **Core Logic:** C (optimized memory management, pointers, and data structures).
* **GUI / Frontend:** C++, Qt 6.9.x
* **IDE:** Qt Creator
* **Platform:** Linux (tested on Fedora)

## Language Syntax (EBNF)

The compiler processes a custom dialect defined by the following Extended Backus-Naur Form (EBNF) grammar. It features specific keywords, logical operators, and a maximum identifier length of 4 characters:

```ebnf
<Program>       ::= "#" "program" <Identifier> ";" <Block>
<Block>         ::= [ "variable" <VarList> ";" ] "start" <StatementList> "stop"
<VarList>       ::= <Identifier> { "," <Identifier> }
<StatementList> ::= { <Statement> [ ";" ] }
<Statement>     ::= <AssignStmt> | <InputStmt> | <OutputStmt> | <CompoundStmt> | 
                    <IfStmt> | <ForStmt>
<AssignStmt>    ::= <Identifier> "->" <Expression>
<InputStmt>     ::= "input" <Identifier>
<OutputStmt>    ::= "output" <Expression>
<CompoundStmt>  ::= "start" <StatementList> "stop"
<IfStmt>        ::= "if" <Condition> "then" <Statement> [ "else" <Statement> ]
<ForStmt>       ::= "for" <Identifier> "->" <Expression> "downto" <Expression> 
                    "do" <Statement>
<Condition>     ::= <LogicTerm> { ("&" | "|") <LogicTerm> }
<LogicTerm>     ::= [ "!" ] <LogicFactor>
<LogicFactor>   ::= "(" <Condition> ")" | <Expression> <RelationOp> <Expression>
<RelationOp>    ::= "eg" | "ne" | ">>" | "<<"
<Expression>    ::= [ "add" | "sub" ] <Term> { ("add" | "sub") <Term> }
<Term>          ::= <Factor> { ("*" | "/" | "%") <Factor> }
<Factor>        ::= <Identifier> | <Number> | "(" <Expression> ")"
<Identifier>    ::= <Letter> [ <Letter> [ <Letter> [ <Letter> ] ] ]
<Letter>        ::= "a" | "b" | ... | "z"
```
###How to Build and Run Locally

Since the project has a Qt graphical interface, the easiest way to build it is using Qt Creator.
1. Clone the Repository

Open your terminal and run:
Bash

git clone [https://github.com/Enfi00/c-compiler.git](https://github.com/Enfi00/c-compiler.git)

2. Build via Qt Creator

    Launch Qt Creator.

    Click File -> Open File or Project...

    Select the project configuration file (CMakeLists.txt or .pro) in the downloaded folder.

    Select a build kit (e.g., Desktop Qt 6.9.x GCC).

    Click the Build button (or Ctrl+B) to compile the project.

3. Run the Application

After a successful build, you can run the application directly from Qt Creator (the Run button or Ctrl+R). Alternatively, you can run the compiled executable directly from the build folder via terminal:
```ebnf
cd build/Desktop_Qt_6_9_3-Debug
./Pl0Studio
```
