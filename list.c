#include <stdio.h>
#include "list.h"

struct foo {
	struct list_head list;

	char *bar;
};

int main(int argc, char **argv)
{
	struct foo a, b, c;
	struct list_head my_list;
	struct list_head *p;

	INIT_LIST_HEAD(&my_list);

	a.bar = "a";
	b.bar = "b";
	c.bar = "c";

	list_add_tail(&a.list, &my_list);
	list_add_tail(&b.list, &my_list);
	list_add_tail(&c.list, &my_list);

	printf("%s:%s[%d] count = %d\n", __FILE__, __func__, __LINE__, list_count(&my_list));
	list_for_each(p, &my_list)
	{
		struct foo *q = container_of(p, struct foo, list);
		printf("%s\n", q->bar);
	}
	return 0;
}
