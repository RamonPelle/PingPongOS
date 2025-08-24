### 1. Explique o objetivo e os parâmetros de cada uma das quatro funções acima.

- `getcontext(&a)`: Salva o contexto atual na variável `a`.
- `setcontext(&a)`: Restaura um contexto salvo anteriormente na variável `a`.
- `swapcontext(&a, &b)`: Salva o contexto atual em `a` e restaura o contexto salvo anteriormente em `b`.
- `makecontext(&a, ...)`: Ajusta alguns valores internos do contexto salvo em `a`.

### 2. Explique o significado dos campos da estrutura `ucontext_t` que foram utilizados no código.

- **`uc_link`**: Vincula um outro contexto ao atual. Quando o contexto atual terminar, o que foi vinculado continua a execução.
- **`uc_stack`**: Estrutura que descreve a pilha usada no contexto. A sigla `ss` vem de _signal stack_.
  - **`ss_sp`**: Endereço base da pilha (_stack_).
  - **`ss_size`**: Número de bytes da pilha.
  - **`ss_flags`**: flags. (Ler a _man page_ de `sigaltstack` para mais detalhes sobre as constantes).

### 3. Explique cada linha do código de `contexts.c` que chame uma dessas funções ou que manipule estruturas do tipo `ucontext_t`.

**Na função `main`:**

- **Linha 67:** `getcontext(&ContextPing)`: Salva o contexto atual (da `main`) na variável `ContextPing`.
- **Linha 68:** `setcontext(&ContextPing)`: Restaura o contexto que está na variável `ContextPing`. Nesse momento, o contexto é o mesmo, então nada muda.
- **A partir da linha 69:** O bloco `if` e as linhas seguintes configuram todas as informações necessárias para a criação da pilha do `ContextPing`.
- **Linha 84:** `makecontext(&ContextPing, ...)`: Ajusta os valores do `ContextPing`. Em vez de a execução continuar de onde o último `getcontext` foi chamado, ela começará na função passada como parâmetro (`BodyPing`), recebendo a string `"    Ping"` como argumento.

_O mesmo processo é repetido para criar e configurar o `ContextPong`._

- **Linha 105:** `swapcontext(&ContextMain, &ContextPing)`: Salva o estado do contexto `main` e ativa o contexto `Ping`.
- **Linha 106:** `swapcontext(&ContextMain, &ContextPong)`: Salva o estado do contexto `main` e ativa o contexto `Pong`. Esta linha é executada apenas depois que `Ping` devolveu o controle para a `main`.

**Na função `BodyPing`:**

- **Linha 30:** `swapcontext(&ContextPing, &ContextPong)`: Salva o estado do contexto `Ping` e ativa o contexto `Pong`. `Pong` continua a execução de onde parou.
- **Linha 33:** `swapcontext(&ContextPing, &ContextMain)`: Salva o estado do contexto `Ping` e ativa o contexto `main`. A `main` continua de onde parou.

**Na função `BodyPong`:**

- **Linha 45:** `swapcontext(&ContextPong, &ContextPing)`: Salva o estado do contexto `Pong` e ativa o contexto `Ping`. `Ping` continua de onde parou.
- **Linha 48:** `swapcontext(&ContextPong, &ContextMain)`: Salva o estado do contexto `Pong` e ativa o contexto `main`. A `main` continua de onde parou.

> \* As trocas (`swap`) entre `Ping` e `Pong` demonstram a **alternância cooperativa**, pois uma tarefa só volta a executar quando a outra libera o processador (não há preempção).

### 4. Para visualizar melhor as trocas de contexto, desenhe o diagrama de tempo dessa execução.
