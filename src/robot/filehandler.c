#include    "goahead.h"
#include	"filehandler.h"
#include 	"tools.h"

static void fileWriteEvent(Webs *wp);
static int avolfileHandler(Webs *wp);

void upload(Webs *wp)
{
	WebsKey         *s;
	WebsUpload      *up;
	char            *upfile;

	websSetStatus(wp, 204);
	websWriteHeaders(wp, -1, 0);
	websWriteHeader(wp, "Content-Type", "text/plain");
	websWriteEndHeaders(wp);

	if (scaselessmatch(wp->method, "POST")) {
		for (s = hashFirst(wp->files); s; s = hashNext(wp->files, s)) {
			up = s->content.value.symbol;
			/* close printf */
			/*websWrite(wp, "FILE: %s\r\n", s->name.value.string);
			websWrite(wp, "FILENAME=%s\r\n", up->filename);
			websWrite(wp, "CLIENT=%s\r\n", up->clientFilename);
			websWrite(wp, "TYPE=%s\r\n", up->contentType);
			websWrite(wp, "SIZE=%d\r\n", up->size);*/
			/*printf("FILE: %s\r\n", s->name.value.string);
		    printf("FILENAME=%s\r\n", up->filename);
			printf("CLIENT=%s\r\n", up->clientFilename);
			printf("TYPE=%s\r\n", up->contentType);
			printf("SIZE=%d\r\n", up->size);*/

			/* point json file */
			if (strcmp(up->clientFilename, "points.json") == 0) {
				upfile = sfmt("%s", FILE_POINTS);
			/* user lua file */
			} else if (is_in(up->clientFilename, ".lua") == 1) {
				upfile = sfmt("%s%s", DIR_USER, up->clientFilename);
			}
			printf("upfile = %s\n", upfile);
			if (rename(up->filename, upfile) < 0) {
				error("Cannot rename uploaded file: %s to %s, errno %d", up->filename, upfile, errno);
			}
			wfree(upfile);
		}
		/*websWrite(wp, "\r\nVARS:\r\n");
		for (s = hashFirst(wp->vars); s; s = hashNext(wp->vars, s)) {
			websWrite(wp, "%s=%s\r\n", s->name.value.string, s->content.value.string);
		}*/
	}
	websRedirect(wp,"/index.html#/programteach");
	websDone(wp);
}

static void fileWriteEvent(Webs *wp)
{

	char    *buf;
	ssize   len, wrote;

	assert(wp);
	assert(websValid(wp));


	if ((buf = walloc(ME_GOAHEAD_LIMIT_BUFFER)) == NULL) {
		websError(wp, HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't get memory");
		return;
	}
	while ((len = websPageReadData(wp, buf, ME_GOAHEAD_LIMIT_BUFFER)) > 0) {
		if ((wrote = websWriteSocket(wp, buf, len)) < 0) {
			break;
		}
		if (wrote != len) {
			websPageSeek(wp, - (len - wrote), SEEK_CUR);
			break;
		}
	}
	wfree(buf);
	if (len <= 0) {
		websDone(wp);
	}
}

static int avolfileHandler(Webs *wp)
{
	WebsFileInfo    info;
	char            *tmp, *date;
	ssize           nchars;
	int             code;

	char    *pathfilename; //带路径的文件名 用于找到对应的文件  
	char    *filenameExt; //文件扩展名 用于 设置 MIME类型  
	char    *filename;      //文件名 用于下载后保存的文件名称  
	char    *disposition; //临时保存 附件 标识  
	char	*ext = '.';
	char	*slash= '/';

	assert(websValid(wp));
	assert(wp->method);
	assert(wp->filename && wp->filename[0]);    

	pathfilename = websGetVar(wp, "pathfilename", NULL);
	printf("pathfilename = %s\n", pathfilename);
	if (pathfilename == NULL)
		return 1;

	//取文件名和扩展名
	//filename =sclone(getUrlLastSplit(sclone(pathfilename),"\\"));
	//filenameExt =sclone(getUrlLastSplit(sclone(filename),"."));
	//filename = "ip.txt";
	filename = strrchr(pathfilename, slash) + 1;
	if (filename)
		printf("The filename: %s\n", filename);
	else {
		websError(wp, HTTP_CODE_NOT_FOUND, "The filename was not found");
		return 1;
	}
	//filenameExt = "txt";
	filenameExt = strrchr(filename, ext) + 1;
	if (filename)
		printf("The filenameExt: %s\n", filenameExt);
	else {
		websError(wp, HTTP_CODE_NOT_FOUND, "The filenameExt was not found");
		return 1;
	}

	if (wp->ext) wfree(wp->ext);

	wp->ext=walloc(1+strlen(filenameExt)+1);
	sprintf(wp->ext,".%s",sclone(filenameExt));
	printf("wp->ext = %s\n", wp->ext);
	//free(filenameExt);
	//filenameExt=NULL;

	if (wp->filename) wfree(wp->filename);
	wp->filename=sclone(pathfilename);
	printf("wp->filename = %s\n", wp->filename);

	if (wp->path) wfree(wp->path);
	wp->path=sclone(pathfilename);
	printf("wp->path = %s\n", wp->path);

	if (websPageIsDirectory(wp)) {
		nchars = strlen(wp->path);
		if (wp->path[nchars - 1] == '/' || wp->path[nchars - 1] == '\\') {
			wp->path[--nchars] = '\0';
		}
		char *websIndex = "testdownload";
		tmp = sfmt("%s/%s", wp->path, websIndex);
		websRedirect(wp, tmp);
		wfree(tmp);
		return 1;
	}

	if (websPageOpen(wp, O_RDONLY | O_BINARY, 0666) < 0) {
		websError(wp, HTTP_CODE_NOT_FOUND, "Cannot open   document for: %s", wp->path);
		return 1;
	}   
	if (websPageStat(wp, &info) < 0) {
		websError(wp, HTTP_CODE_NOT_FOUND, "Cannot stat page for URL");
		return 1;
	}

	code = 200;
	if (wp->since && info.mtime <= wp->since) {
		code = 304;
	}

	websSetStatus(wp, code);
	websWriteHeaders(wp, info.size, 0);
	disposition = walloc(20+strlen(filename)+1);
	//设置下载文件的名称
	sprintf(disposition,"attachment;filename=%s",sclone(filename));
	websWriteHeader(wp, "Content-Disposition", sclone(disposition));

	//free(filename);
	free(disposition);
	//filename=NULL;
	disposition=NULL;  
	if ((date = websGetDateString(&info)) != NULL) {
		websWriteHeader(wp, "Last-modified", "%s", date);
		wfree(date);
	}

	websWriteEndHeaders(wp);

	/*
	   All done if the browser did a HEAD request
	 */
	if (smatch(wp->method, "HEAD")) {
		websDone(wp);
		return 1;
	}
	websSetBackgroundWriter(wp, fileWriteEvent);

	return 1;
}

void download(Webs *wp, char *path, char *query){
	WebsHandlerProc service = (*wp).route->handler->service; 
//	(*wp).route->handler->close = (*avolfileClose);
	(*wp).route->handler->service =(*avolfileHandler); 
	(*wp).route->handler->service(wp); 
	(*wp).route->handler->service= service; 
}
