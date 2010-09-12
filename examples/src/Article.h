#import <Array.h>
#import <String.h>

typedef struct {
	size_t id;
	String title;
} Article;

#define Article(id, title) (Article) { id, title }

typedef Array(Article, Articles);
