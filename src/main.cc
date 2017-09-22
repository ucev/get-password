#ifndef __MY_GETPASS_NODE
#define __MY_GETPASS_NODE

#include <errno.h>
#include <node.h>
#include <termios.h>
#include <unistd.h>
#include <uv.h>

#include <iostream>

#define ECHOFLAGS (ECHO | ECHOE | ECHOK | ECHONL)
#define ERROR1 "Cannot get the attribution of the terminal"
#define ERROR2 "Cannot set the attribution of the terminal"

namespace _getpass {
  using namespace v8;
  using namespace std;

  int set_disp_mode (int fd, int option) {
    int err;
    struct termios term;
    if (tcgetattr(fd, &term) == -1) {
      /**
       * Error Handler
       */
      //perror("Cannot get the attribution of the terminal");
      return 1;
    }
    if (option) {
      term.c_lflag |= ECHOFLAGS;
    } else {
      term.c_lflag &=~ ECHOFLAGS;
    }
    err = tcsetattr(fd, TCSAFLUSH, &term);
    if (err == -1 && err == EINTR) {
      //perror("Cannot set the attribution of the terminal");
      return 2;
    }
    return 0;
  }

  struct DelayBaton {
    uv_write_t write;
    uv_tty_t tty_w;
    uv_tty_t tty;
    Persistent<Function> callback;
    // not used
    int error;
    string prompt;
    char *passwd;
  };

  void ReadAllocCallback(uv_handle_t*, size_t, uv_buf_t*);
  void ReadReadCallback(uv_stream_t*, ssize_t, const uv_buf_t*);
  void WriteNewLineCallback(uv_write_t*, int);
  void WritePromptCallback(uv_write_t*, int);

  void Delay(const FunctionCallbackInfo<Value>& args) {
    Isolate *isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    DelayBaton *baton = new DelayBaton;
    uv_loop_t *loop = uv_default_loop();
    int cbIndex = 0;
    string prompt = "请输入密码：";
    if (!args[0]->IsFunction()) {
      cbIndex = 1;
      if (args[0]->IsObject()) {
        Local<Object> obj = args[0]->ToObject();
        Local<String> key = String::NewFromUtf8(isolate, "prompt");
        Maybe<bool> maybeHas = obj->Has(context, key);
        if (maybeHas.IsJust()) {
          String::Utf8Value sval(obj->Get(context, key).ToLocalChecked());
          prompt.assign(*sval);
        }
      }
    }
    if (!args[cbIndex]->IsFunction()) {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "参数错误")));
      delete baton;
      return;
    }
    Local<Function> cb = Local<Function>::Cast(args[cbIndex]);
    baton->callback.Reset(isolate, cb);
    baton->error = 0;
    set_disp_mode(STDIN_FILENO, 0);
    uv_buf_t buf;
    buf.base = (char*)prompt.c_str();
    buf.len = strlen(buf.base);
    uv_tty_init(loop, &baton->tty_w, 1, 0);
    baton->write.data = baton;
    uv_write(&baton->write, (uv_stream_t*)&baton->tty_w, &buf, 1, WritePromptCallback);
  }

  void ReadAllocCallback(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    //
    suggested_size = 500;
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
  }

  void ReadReadCallback(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread == 0) {
      return;
    }
    if (buf->base[nread - 1] == '\n') {
      buf->base[nread - 1] = '\0';
    }
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope handleScope(isolate);
    uv_read_stop(stream);
    DelayBaton *baton = static_cast<DelayBaton*>(stream->data);
    baton->passwd = buf->base;
    string newLine = "\n";
    uv_buf_t wbuf;
    wbuf.base = (char*)newLine.c_str();
    wbuf.len = strlen(wbuf.base);
    uv_write(&baton->write, (uv_stream_t*)&baton->tty_w, &wbuf, 1, WriteNewLineCallback);
  }

  void WriteNewLineCallback(uv_write_t *req, int status) {
    // no error handler at current time
    if (status == 0) {
      Isolate *isolate = Isolate::GetCurrent();
      HandleScope handleScope(isolate);
      DelayBaton *baton = static_cast<DelayBaton*>(req->data);
      const int argc = 2;
      Local<Value> argv[] = {
        Undefined(isolate),
        Local<Value>(String::NewFromUtf8(isolate, baton->passwd))
      };
      set_disp_mode(STDIN_FILENO, 1);
      Local<Function>::New(isolate, baton->callback)->Call(isolate->GetCurrentContext()->Global(),argc, argv);
      baton->callback.Reset();
      delete baton;
    }
  }

  void WritePromptCallback(uv_write_t *req, int status) {
    // no error handler at current time
    if (status == 0) {
      Isolate *isolate = Isolate::GetCurrent();
      HandleScope handleScope(isolate);
      DelayBaton *baton = static_cast<DelayBaton*>(req->data);
      uv_loop_t *loop = uv_default_loop();
      uv_tty_init(loop, &baton->tty, 0, 1);
      baton->tty.data = baton;
      uv_read_start((uv_stream_t*)&baton->tty, ReadAllocCallback, ReadReadCallback);
    }
  }

  void init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "getpass", Delay);
  }

  NODE_MODULE(getpass, init)
}

#endif
