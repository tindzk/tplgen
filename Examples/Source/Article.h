#import <Array.h>
#import <String.h>

record(Article) {
	size_t id;
	RdString title;
};

#define Article(id, title) (Article) { id, title }

Array(Article, Articles);
