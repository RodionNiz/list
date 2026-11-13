#include "list.cpp"

int main ()
{
    size_t len = 16;
    List list1 = {}; 

    ListCtorCall(list1, len);

    InsertListLg (&list1, 0, 10);
    InsertListLg (&list1, 1, 50);
    InsertListLg (&list1, 1, 15);
    InsertListLg (&list1, 2, 35);
    InsertListLg (&list1, 4, 80);
    InsertTail (&list1, 80);
    InsertTail (&list1, 80);
    InsertTail (&list1, 80);
    InsertTail (&list1, 80);
    list1.data [7].next = 4;
    InsertTail (&list1, 80);
    InsertNext (&list1, 1000, 80);



    listElem_t value = GLOBAL_LIST_POISON;
    ExtrHead (&list1, &value);
    
    ExtrListLg (&list1, 10, &value);

    ExtrList (&list1, 2, &value);

    ListDtor (&list1);

    printf ("Extrped = %lg", value);
}