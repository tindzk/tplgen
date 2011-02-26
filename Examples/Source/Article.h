#import <Array.h>
#import <String.h>

record(Article) {
	size_t id;
	ProtString title;
};

#define Article(id, title) (Article) { id, title }

Array(Article, Articles);
