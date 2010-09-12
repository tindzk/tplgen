#import <String.h>
#import <Signal.h>
#import <ExceptionManager.h>

#import "Article.h"
#import "Template.h"

ExceptionManager exc;

void printListing(String *out, Articles *articles) {
	ListingTemplate tpl;
	tpl.articles = articles;

	Template_Listing(&tpl, out);
}

void printArticle(String *out, Article article) {
	ArticleTemplate tpl;
	tpl.article = article;

	Template_Article(&tpl, out);
}

int main(__unused int argc, __unused char *argv[]) {
	ExceptionManager_Init(&exc);

	Memory0(&exc);
	String0(&exc);
	Signal0(&exc);

	Articles *articles;

	Array_Init(articles, 3);

	Array_Push(articles, Article(0, String("First article")));
	Array_Push(articles, Article(1, String("Second article")));
	Array_Push(articles, Article(2, String("Third article")));

	String res = HeapString(0);

	String_Print(String("Listing template:\n"));
	printListing(&res, articles);
	String_Print(res);

	String_Print(String("\n"));
	res.len = 0;

	String_Print(String("Article template:\n"));
	printArticle(&res, articles->buf[1]);
	String_Print(res);

	String_Print(String("\n"));

	Array_Destroy(articles);
	String_Destroy(&res);

	return EXIT_SUCCESS;
}
