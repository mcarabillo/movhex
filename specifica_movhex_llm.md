Ecco una sintesi delle specifiche del documento **"Prova finale di
algoritmi e strutture dati 2024-2025"**, con incluse tutte le formule
matematiche:

------------------------------------------------------------------------

## Contesto

-   La compagnia Movhex vuole un programma per calcolare rotte ottimali
    di veicoli su una **mappa di esagoni** (piastrellata rettangolare
    con righe e colonne).
-   Ogni esagono è identificato da coordinate (colonna, riga), a partire
    da (0,0) in basso a sinistra.
-   Ogni esagono ha un **costo di uscita via terra** (intero tra 1 e
    100, oppure 0 se intransitabile).
-   Sono possibili **rotte aeree monodirezionali** (max 5 uscenti da
    ogni esagono), ciascuna con un proprio costo di attraversamento.

------------------------------------------------------------------------

## Comandi del programma

### 1. `init ⟨n. colonne⟩ ⟨n. righe⟩`

-   Inizializza la mappa con dimensioni specificate.\
-   Tutti i costi iniziali = 1, nessuna rotta aerea.\
-   Output: `OK`.

------------------------------------------------------------------------

### 2. `change_cost ⟨x⟩ ⟨y⟩ ⟨v⟩ ⟨raggio⟩`

-   Modifica i costi di un insieme di esagoni attorno a (x, y).\
-   Si usa la distanza fra esagoni definita come:

\[ DistEsagoni((x_a, y_a), (x_b, y_b)) =
`\text{numero minimo di esagoni da percorrere da }`{=tex} (x_a,y_a)
`\text{ a }`{=tex} (x_b,y_b) \]

-   La variazione di costo è applicata a ogni esagono ((x_e, y_e)) con:

\[ costo(x_e, y_e) = costo(x_e, y_e) + `\left`{=tex}`\lfloor `{=tex}v
`\times `{=tex}`\max`{=tex}`\left`{=tex}(0,
`\frac{raggio - DistEsagoni(x_e, y_e, x, y)}{raggio}`{=tex}`\right`{=tex})
`\right`{=tex}`\rfloor`{=tex} \]

-   Valori di ⟨v⟩ compresi tra -10 e 10.\
-   Output: `KO` se (x,y) non è valido o se raggio=0, altrimenti `OK`.

------------------------------------------------------------------------

### 3. `toggle_air_route ⟨x1⟩ ⟨y1⟩ ⟨x2⟩ ⟨y2⟩`

-   Aggiunge o rimuove una rotta aerea da (x1,y1) a (x2,y2).\
-   Se aggiunta: il costo = **media arrotondata per difetto** tra:
    -   i costi di tutte le rotte aeree già uscenti da (x1,y1)\
    -   e il costo di uscita via terra dell'esagono (x1,y1).\
-   Output:
    -   `OK` se i due esagoni sono validi e (x1,y1) non ha già 5 rotte
        aeree.\
    -   `KO` altrimenti.

------------------------------------------------------------------------

### 4. `travel_cost ⟨xp⟩ ⟨yp⟩ ⟨xd⟩ ⟨yd⟩`

-   Restituisce il **costo minimo** per andare da (xp,yp) a (xd,yd)
    attraverso rotte terrestri e/o aeree.\
-   Regole:
    -   Il costo di uscita dell'esagono di destinazione **non si
        paga**.\
    -   Se partenza = destinazione → costo = 0.\
    -   Se si attraversa una rotta aerea, **non si paga** il costo di
        uscita via terra dell'esagono sorgente della rotta.\
-   Output: costo minimo trovato, oppure `-1` se le coordinate non sono
    valide o la destinazione è irraggiungibile.

------------------------------------------------------------------------

## Esempio fornito

-   Inizializzazione: `init 100 100 → OK`.\
-   Modifica costi: `change_cost 10 20 -10 5 → OK` (rende intransitabile
    una zona).\
-   Calcolo rotte: `travel_cost 0 0 20 0 → 20`.\
-   Aggiunta rotta aerea: `toggle_air_route 0 0 20 0 → OK`, ora
    `travel_cost 0 0 20 0 → 1`.\
-   Gestione errori: se coordinate non valide → `KO` o `-1`.

------------------------------------------------------------------------

Vuoi che ti trasformi questa sintesi in uno **schema visuale
(diagramma/flowchart dei comandi e formule)** per rendere la logica più
chiara?
