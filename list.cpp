#include <cassert>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef double listElem_t;
typedef size_t physInd_t;
typedef size_t logicInd_t;

#define ON_DBG

#ifdef ON_DBG
#define DBG(...) __VA_ARGS__
#endif

#ifdef ON_DBG
#define ListCtorCall(list, len) \
        do{\
            ListCtor (&list, len DBG(, __FILE__, #list, __FUNCTION__, __LINE__));\
        }\
        while (0)

#define VERIFY_LIST(list, okDmp) \
        do{\
            if (VerList (list, __LINE__, okDmp) != 0)\
            {\
                abort ();\
            }\
        }\
        while (0)
#else
#define ListCtorCall(list, len) \
        ListCtor (&list, len);

#define VERIFY_LIST(list)
#endif

#ifdef ON_DBG
    #define GET_NEXT(list, index)  \
        SNext (list, index)
#else
    #define GET_NEXT(list, index)
        Next (list, index)
#endif


#ifdef ON_DBG
    #define GET_PREV(list, index)  \
        SPrev (list, index)
#else
    #define GET_PREV(list, index)  \
        Prev (list, index)
#endif


#ifdef ON_DBG
    #define GET_VALUE(list, index)  \
        SValue (list, index)
#else
    #define GET_VALUE(list, index)  \
        Value (list, index)
#endif
    


struct listElem_s
{
    listElem_t value;
    physInd_t next;
    physInd_t prev;
};

struct List 
{
    listElem_s* data;
    size_t size;
    physInd_t firstFree;

    DBG(const char* file;
        const char* name;
        const char* func;
        size_t line;)
};

enum ErrCodes : size_t
{
    ListOk = 0,
    NoLupErr = 1 << 0,      //0x1
    InvalidValue = 1 << 1,  //0x2
    InvalidNext = 1 << 2,   //0x4
    InvalidPrev = 1 << 3,   //0x8
    InvalidBind = 1 << 4    //0x16
};

const listElem_t GLOBAL_LIST_POISON = NAN;

void ListCtor (List* newList, size_t len DBG(, const char* file, const char* name, const char* func, size_t line));
void ListDtor (List* list);

static inline physInd_t   GetFree (List* const list);
static inline void        PutFree (List* const list, physInd_t newFree);

static inline physInd_t*  Next    (List* list, physInd_t index);
static inline physInd_t*  SNext   (List* list, physInd_t index);

static inline physInd_t*  Prev    (List* list, physInd_t index);
static inline physInd_t*  SPrev   (List* list, physInd_t index);

static inline listElem_t* Value   (List* list, physInd_t index);
static inline listElem_t* SValue  (List* list, physInd_t index);

static inline void BindElems      (List* list, physInd_t prev, physInd_t next);

physInd_t FindByLogic (List* const list, logicInd_t nextLogic);


struct InsertCtx
{
    List* list;
    listElem_t value;
    physInd_t indToWrite;
    physInd_t next;
    physInd_t prev;
};
physInd_t InsertNext   (List* list, physInd_t index, listElem_t value);
physInd_t InsertListLg (List* list, logicInd_t index, listElem_t value);
physInd_t InsertHead   (List* list, listElem_t value);
physInd_t InsertTail   (List* list, listElem_t value);
static void DoInsert   (InsertCtx* ctx);


struct ExtrCtx
{
    List* list;
    listElem_t* value;
    physInd_t indToExtr;
    physInd_t next;
    physInd_t prev;
};
physInd_t ExtrList   (List* list, physInd_t index, listElem_t* value);
physInd_t ExtrListLg (List* list, logicInd_t index, listElem_t* value);
physInd_t ExtrNextLg (List* list, logicInd_t index, listElem_t* value);
physInd_t ExtrTail   (List* list, listElem_t* value);
physInd_t ExtrHead   (List* list, listElem_t* value);
static void DoExtr   (ExtrCtx* ctx);


ErrCodes VerList      (List* list, size_t assertLine, int dmpOk);
void ListDump         (List* list);
void GraphListDump    (List* list, physInd_t invalid, size_t assertLine, size_t errCode);
void ListTextFileDump (List* list, FILE* file);

size_t GLOBAL_DUMP_COUNT = 0;

void ListCtor (List* newList, size_t len DBG(, const char* file, const char* name, const char* func, size_t line))
{
    newList->size = len;
    newList->data = (listElem_s*) calloc (len, sizeof (newList->data [0]));
    if (newList->data == nullptr)
    {
        printf ("List data allocation err\n");
        return;
    }

    for (size_t i = 1; i < len - 1; i++)
    {
        *GET_NEXT (newList, i) = i + 1;
        *GET_VALUE (newList, i) = GLOBAL_LIST_POISON;
    }
    newList->firstFree = 1;
    *GET_NEXT (newList, len - 1) = len - 1;
    *GET_VALUE (newList, len - 1) = GLOBAL_LIST_POISON;

    #ifdef ON_DBG
    newList->file = file;
    newList->func = func;
    newList->line = line;
    newList->name = name;
    #endif

    VERIFY_LIST (newList, 1);
}


void ListDtor (List* list)
{
    VERIFY_LIST (list, 0);

    for (size_t i = list->size - 1; i != SIZE_MAX; i--)
    {
        *(GET_PREV(list, i)) = 0;
        *GET_NEXT (list, i) = 0;
        *GET_VALUE (list, i) = GLOBAL_LIST_POISON;
    }

    free (list->data);
}


physInd_t InsertNext (List* list, physInd_t index, listElem_t value)
{
    VERIFY_LIST (list, 1);

    if (index == SIZE_MAX)
    {
        return InsertHead (list, value);
    }
    physInd_t indToWrite = GetFree (list);

    InsertCtx ctx = {.list = list, .value = value, .indToWrite = indToWrite, .next = *GET_NEXT (list, index), .prev = index};
    DoInsert (&ctx);

    VERIFY_LIST (list, 1);
    return indToWrite;
}

physInd_t InsertListLg (List* list, logicInd_t index, listElem_t value)
{
    VERIFY_LIST (list, 1);
    if (index == 0)
    {
        return InsertHead (list, value);;
    }

    physInd_t indToWrite = GetFree (list);
    if (indToWrite == list->size) 
    {
        printf ("No free space\n");
        return indToWrite;
    }

    physInd_t prev = FindByLogic (list, index - 1);    
    InsertCtx ctx = {.list = list, .value = value, .indToWrite = indToWrite, .next = *GET_NEXT (list, prev), .prev = prev};
    DoInsert (&ctx);

    VERIFY_LIST (list, 1);
    return indToWrite;
}

physInd_t InsertHead (List* list, listElem_t value)
{
    VERIFY_LIST (list, 1);

    physInd_t indToWrite = GetFree (list);
    if (indToWrite == list->size) 
    {
        printf ("No free space\n");
        return indToWrite;
    }

    InsertCtx ctx = {.list = list, .value = value, .indToWrite = indToWrite, .next = *GET_NEXT (list, 0), .prev = 0};
    DoInsert (&ctx);

    VERIFY_LIST (list, 1);
    return indToWrite;
}

physInd_t InsertTail (List* list, listElem_t value)
{
    VERIFY_LIST (list, 1);

    physInd_t indToWrite = GetFree (list);
    if (indToWrite == list->size) 
    {
        printf ("No free space\n");
        return indToWrite;
    }

    InsertCtx ctx = {.list = list, .value = value, .indToWrite = indToWrite, .next = 0, .prev = *GET_PREV (list, 0)};
    DoInsert (&ctx);

    VERIFY_LIST (list, 1);
    return indToWrite;
}

static void DoInsert (InsertCtx* ctx)
{
    VERIFY_LIST (ctx->list, 0);
    *GET_VALUE (ctx->list, ctx->indToWrite) = ctx->value;
    BindElems (ctx->list, ctx->prev, ctx->indToWrite);
    BindElems (ctx->list, ctx->indToWrite, ctx->next);
    VERIFY_LIST (ctx->list, 0);
}


physInd_t ExtrList (List* list, physInd_t indToExtr, listElem_t* value)
{
    VERIFY_LIST (list, 1);
    ExtrCtx ctx = {.list = list, .value = value, .indToExtr = indToExtr, .next = *GET_NEXT (list, indToExtr), .prev = *GET_PREV (list, indToExtr)};
    DoExtr (&ctx);

    VERIFY_LIST (list, 1);
    return indToExtr;
}


physInd_t ExtrListLg (List* list, logicInd_t index, listElem_t* value)
{
    VERIFY_LIST (list, 1);

    physInd_t indToExtr = FindByLogic (list, index);
    if (indToExtr == list->size) 
    {
        printf ("No elem to Extr\n");
        return 0;
    }
    ExtrCtx ctx = {.list = list, .value = value, .indToExtr = indToExtr, .next = *GET_NEXT (list, indToExtr), .prev = *GET_PREV (list, indToExtr)};
    DoExtr (&ctx);

    VERIFY_LIST (list, 1);
    return indToExtr;
}


physInd_t ExtrNextLg (List* list, logicInd_t index, listElem_t* value)
{
    VERIFY_LIST (list, 1);

    if (index == SIZE_MAX)
    {
        ExtrHead (list, value);
    }

    physInd_t prev = FindByLogic (list, index);
    if (prev == list->size) 
    {
        printf ("No prev element\n");
        return 0;
    }
    physInd_t indToExtr = *GET_NEXT (list, prev);
    ExtrCtx ctx = {.list = list, .value = value, .indToExtr = indToExtr, .next = *GET_NEXT (list, indToExtr), .prev = prev};
    DoExtr (&ctx);

    VERIFY_LIST (list, 1);
    return indToExtr;
}


physInd_t ExtrTail (List* list, listElem_t* value)
{
    VERIFY_LIST (list, 1);
    
    physInd_t indToExtr = *GET_PREV (list, 0);
    physInd_t prev = *GET_PREV (list, indToExtr);

    ExtrCtx ctx = {.list = list, .value = value, .indToExtr = indToExtr, .next = 0, .prev = prev};
    DoExtr (&ctx);

    VERIFY_LIST (list, 1);
    return indToExtr;
}


physInd_t ExtrHead (List* list, listElem_t* value)
{
    VERIFY_LIST (list, 1);

    physInd_t indToExtr = *GET_NEXT (list, 0);

    ExtrCtx ctx = {.list = list, .value = value, .indToExtr = indToExtr, .next = *GET_NEXT (list, indToExtr), .prev = 0};
    DoExtr (&ctx);

    VERIFY_LIST (list, 1);
    return indToExtr;
}


static void DoExtr (ExtrCtx* ctx)
{
    VERIFY_LIST (ctx->list, 0);

    *ctx->value = *GET_VALUE (ctx->list, ctx->indToExtr);
    *GET_VALUE (ctx->list, ctx->indToExtr) = GLOBAL_LIST_POISON;
    BindElems (ctx->list, ctx->prev, ctx->next);
    PutFree (ctx->list, ctx->indToExtr);

    VERIFY_LIST (ctx->list, 0);
}  


static inline physInd_t GetFree (List* const list)
{
    VERIFY_LIST (list, 0);

    physInd_t free = list->firstFree;
    list->firstFree = *GET_NEXT (list, free);
    if (free == list->size)
    {
        printf ("List overflow\n");
    }

    VERIFY_LIST (list, 0);
    return free;
}


static inline physInd_t* Next (List* list, physInd_t index)
{
    if (index >= list->size)
    {
        printf ("Invalid index in Next\n");
        return &list->data [0].next;;
    }

    return &list->data [index].next;
}

static inline physInd_t* SNext (List* list, physInd_t index)
{
    physInd_t* tempInd = Next (list, index);
    if (!tempInd) 
    {
        printf ("next ptr is 0\n");
        abort  ();
    }

    return tempInd;
}

static inline physInd_t* Prev (List* list, physInd_t index)
{
    if (index >= list->size)
    {
        printf ("Invalid index\n");
        return &list->data [0].prev;;
    }

    return &list->data [index].prev;
}

static inline physInd_t* SPrev (List* list, physInd_t index)
{
    physInd_t* tempInd = Prev (list, index);
    if (!tempInd) 
    {
        printf ("prev ptr is 0\n");
        abort  ();
    }

    return tempInd;
}

static inline listElem_t* Value (List* list, physInd_t index)
{
    if (index >= list->size)
    {
        printf ("Invalid index\n");
        return &list->data [0].value;;
    }

    return &list->data [index].value;
}

static inline listElem_t* SValue (List* list, physInd_t index)
{
    listElem_t* tempInd = Value (list, index);
    if (!tempInd) 
    {
        printf ("value ptr is 0\n");
        abort  ();
    }

    return tempInd;
}

static inline void PutFree (List* const list, physInd_t newFree)
{
    VERIFY_LIST (list, 0);

    *GET_NEXT (list, newFree) = list->firstFree;
    *GET_PREV (list, newFree) = 0;
    list->firstFree = newFree;

    VERIFY_LIST (list, 0);
}


physInd_t FindByLogic (List* const list, logicInd_t logicInd)
{
    VERIFY_LIST (list, 0);

    if (logicInd >= list->size)
    {
        printf ("Invalid logicInd\n");
        return list->size;
    }

    physInd_t tempInd = *GET_NEXT (list, 0);
    for (physInd_t i = 0; i < logicInd; i++)
    {
        if (tempInd == 0)
        {
            printf ("Invalid logicInd\n");
            return list->size;
        }
        tempInd = *GET_NEXT (list, tempInd);
    }

    VERIFY_LIST (list, 0);
    return tempInd;
}


static inline void BindElems (List* list, physInd_t prev, physInd_t next)
{
    *GET_NEXT (list, prev) = next;
    *GET_PREV (list, next) = prev;
}

//TODO - readme 

ErrCodes VerList (List* list, size_t assertLine, int dmpOk)
{
    assert (list != nullptr);
    assert (list->data != nullptr);

    physInd_t tempInd = 0;
    for (logicInd_t i = 0; i < list->size; i++)
    {
        if (isnan (*GET_VALUE (list, tempInd))) 
        {
            GraphListDump (list, tempInd, assertLine, InvalidValue);
            return InvalidValue;
        }

        if (*GET_NEXT (list, tempInd) >= list->size) 
        {
            GraphListDump (list, tempInd, assertLine, InvalidNext);
            return InvalidNext;
        }

        if (*GET_PREV (list, tempInd) >= list->size) 
        {
            GraphListDump (list, tempInd, assertLine, InvalidPrev);
            return InvalidPrev;
        }

        if (tempInd !=  *GET_PREV (list, *GET_NEXT (list, tempInd)))
        {
            GraphListDump (list, tempInd, assertLine, InvalidBind);
            return InvalidBind;
        }

        tempInd = *GET_NEXT (list, tempInd);
        if (tempInd == 0) break;
    }

    if (tempInd)
    {
        GraphListDump (list, 0, assertLine, NoLupErr);
        return NoLupErr;
    }

    if (dmpOk)
    {
        GraphListDump (list, list->size, assertLine, ListOk);
    }

    return ListOk;
}


void ListDump (List* list)
{
    printf ("List \'%s\' [%p], created in file \'%s\', function \'%s\', line %zu \n", list->name, list, list->file, list->func, list->size);

    printf ("{\n");
    for (physInd_t i = 0; i < list->size; i++)
    {
        if (i == *GET_NEXT (list, 0))
        {
            printf ("\t%zu\t[%3lg], next is %zu, prev is %zu head\n", i, *GET_VALUE (list, i), *GET_NEXT (list, i), *GET_PREV (list, i));
        }

        else if (i == *GET_PREV (list, 0))
        {
            printf ("\t%zu\t[%3lg], next is %zu, prev is %zu tail\n", i, *GET_VALUE (list, i), *GET_NEXT (list, i), *GET_PREV (list, i));
        }

        else
        {
            printf ("\t%zu\t[%3lg], next is %zu, prev is %zu\n", i, *GET_VALUE (list, i), *GET_NEXT (list, i), *GET_PREV (list, i));
        }
    }
    printf ("}\n");
    getchar ();
}


void GraphListDump (List* list, physInd_t invalid, size_t assertLine, size_t errCode)
{
    FILE* graphFile = fopen ("grDump.txt", "w");
    if (graphFile == nullptr)
    {
        printf ("graph file open err\n");
        return;
    }

    fprintf (graphFile, "digraph G{\nrankdir=LR\n"); 

    fprintf (graphFile, "\tnode%zu[shape=Mrecord, style=filled, fillcolor=\"lightblue\" label=\"index = %zu |value = %lg| next = %zu | prev = %zu\"];\n", 0LL, 0LL, *GET_VALUE (list, 0), *GET_NEXT (list, 0), *GET_PREV (list, 0));
    if (invalid == 0) fprintf (graphFile, "\tnode%zu[shape=Mrecord, style=filled, fillcolor=\"red\" label=\"index = %zu |value = %lg| next = %zu | prev = %zu\"];\n", 0LL, 0LL, *GET_VALUE (list, 0), *GET_NEXT (list, 0), *GET_PREV (list, 0));

    for (physInd_t nNode = 1; nNode < list->size; nNode++)
    {
        if      (isnan (*GET_VALUE (list, nNode)))  fprintf (graphFile, "\tnode%zu[shape=Mrecord, style=filled, fillcolor=\"lightyellow\" label=\" index = %zu |value = %lg| next = %zu | prev = %zu\"];\n", nNode, nNode, *GET_VALUE (list, nNode), *GET_NEXT (list, nNode), *GET_PREV (list, nNode));
        else if (nNode == *GET_NEXT (list, 0))      fprintf (graphFile, "\tnode%zu[shape=Mrecord, style=filled, fillcolor=\"lightgrey\"   label=\" index = %zu |value = %lg| next = %zu | prev = %zu\"];\n", nNode, nNode, *GET_VALUE (list, nNode), *GET_NEXT (list, nNode), *GET_PREV (list, nNode));
        else if (nNode == *GET_PREV (list, 0))      fprintf (graphFile, "\tnode%zu[shape=Mrecord, style=filled, fillcolor=\"lightgrey\"   label=\" index = %zu |value = %lg| next = %zu | prev = %zu\"];\n", nNode, nNode, *GET_VALUE (list, nNode), *GET_NEXT (list, nNode), *GET_PREV (list, nNode));
        else                                    fprintf (graphFile, "\tnode%zu[shape=Mrecord, style=filled, fillcolor=\"lightgreen\"  label=\" index = %zu |value = %lg| next = %zu | prev = %zu\"];\n", nNode, nNode, *GET_VALUE (list, nNode), *GET_NEXT (list, nNode), *GET_PREV (list, nNode));
        if      (nNode == invalid)              fprintf (graphFile, "\tnode%zu[shape=Mrecord, style=filled, fillcolor=\"red\"         label=\"index = %zu | value = %lg| next = %zu | prev = %zu\"];\n", nNode, nNode, *GET_VALUE (list, nNode), *GET_NEXT (list, nNode), *GET_PREV (list, nNode));
    }

    for (physInd_t nNode = 0; nNode < list->size; nNode++)
    {
        physInd_t next = *GET_NEXT (list, nNode);
        if (nNode != list->size - 1) fprintf (graphFile, "\tnode%zu->node%zu [style = invis, weight = 10];\n", nNode, nNode + 1);
        if (nNode != invalid)        fprintf (graphFile, "\tnode%zu->node%zu;\n", nNode, next);
    }

    if      (errCode == InvalidBind) fprintf (graphFile, "\tnode%zu->node%zu [color=\"red\", fillcolor=\"red\"];\n", invalid, *GET_NEXT (list, invalid));
    else if (invalid != list->size)  fprintf (graphFile, "\tnode%zu->node%zu;\n", invalid, *GET_NEXT (list, invalid));

    fprintf (graphFile, "}\n");
    fclose  (graphFile);

    char* dumpCmd = (char*) calloc (100, sizeof(char));
    sprintf (dumpCmd, "Dot C:/DEDdop/list/grDump.txt -Tpng -o C:/DEDdop/list/GraphDump/GraphDump%zu.png", GLOBAL_DUMP_COUNT);
    system  (dumpCmd);
    free    (dumpCmd);

    FILE* dmpHtml = (GLOBAL_DUMP_COUNT == 0) ? fopen ("dmpHtml.html", "w+") : fopen ("dmpHtml.html", "a");
    if (dmpHtml == nullptr)
    {
        printf ("html file open err\n");
        return;
    }

    fprintf (dmpHtml, "<pre>\n");
    fprintf (dmpHtml, "    <h1> ListDump: assert line %zu, invalid elem %zu, errCode %zu</h1>\n", assertLine, invalid, errCode);

    ListTextFileDump (list, dmpHtml);

    fprintf (dmpHtml, "\n"
                     "\t\t<img src = \"GraphDump/GraphDump%zu.png\">\n"
                     "\t<hr>\n"
                     "<!-- ------------------------------------------------------------ -->\n", GLOBAL_DUMP_COUNT);

    fprintf (dmpHtml, "</pre>\n\n");

    GLOBAL_DUMP_COUNT++;
    fclose (dmpHtml);
}


void ListTextFileDump (List* list, FILE* file)
{
    fprintf (file, "List \'%s\' [%p], created in file \'%s\', function \'%s\', line %zu \n", list->name, list, list->file, list->func, list->size);

    fprintf (file, "{\n");
    for (physInd_t i = 0; i < list->size; i++)
    {
        if (i == *GET_NEXT (list, 0))
        {
            fprintf (file, "\t%zu\t[%3lg], next is %zu, prev is %zu head\n", i, *GET_VALUE (list, i), *GET_NEXT (list, i), *GET_PREV (list, i));
        }

        else if (i == *GET_PREV (list, 0))
        {
            fprintf (file, "\t%zu\t[%3lg], next is %zu, prev is %zu tail\n", i, *GET_VALUE (list, i), *GET_NEXT (list, i), *GET_PREV (list, i));
        }

        else
        {
            fprintf (file, "\t%zu\t[%3lg], next is %zu, prev is %zu\n", i, *GET_VALUE (list, i), *GET_NEXT (list, i), *GET_PREV (list, i));
        }
    }
    fprintf (file, "}\n");
}