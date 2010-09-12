#import <String.h>

#import "Article.h"

typedef struct {
	Article article;
} ArticleTemplate;

typedef struct {
	Articles *articles;
} ListingTemplate;

void Template_Article(ArticleTemplate *this, String *res);
void Template_Listing(ListingTemplate *this, String *res);
