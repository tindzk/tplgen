#import <String.h>
#import <Exception.h>

#import "Article.h"
#import "Template.h"

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
	Articles *articles = Articles_New(4);

	Articles_Push(&articles, Article(0, $("First article")));
	Articles_Push(&articles, Article(1, $("Second article")));
	Articles_Push(&articles, Article(2, $("Third article")));

	String res = HeapString(0);

	String_Print($("Listing template:\n"));
	printListing(&res, articles);
	String_Print(res);

	String_Print($("\n\n"));
	res.len = 0;

	String_Print($("Article template:\n"));
	printArticle(&res, articles->buf[1]);
	String_Print(res);

	String_Print($("\n"));

	Articles_Free(articles);
	String_Destroy(&res);

	return ExitStatus_Success;
}
