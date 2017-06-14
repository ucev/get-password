#ifndef __MY_GETPASS_NODE
#define __MY_GETPASS_NODE

#include <errno.h>
#include <node.h>
#include <termios.h>
#include <unistd.h>
#include <uv.h>

#include <iostream>
#include <vector>

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

  string getPassword(string prompt) {
    const int MAX_SIZE = 500;
    int c;
    set_disp_mode(STDIN_FILENO, 0);
    vector<char> passwd;
    // string 比较
    if (prompt == "") {
      prompt = "请输入密码";
    }
    cout << prompt << ": " << endl;
    c = cin.get();
    while (c != '\n' && c != '\r' && passwd.size() < MAX_SIZE) {
      passwd.push_back(c);
      c = cin.get();
    }
    /**
     * 这里会不会调用尚有疑问
     * error: cannot use 'try' with exceptions disabled
     */
    set_disp_mode(STDIN_FILENO, 1);
    return string(passwd.begin(), passwd.end());
  }

  struct DelayBaton {
    uv_work_t request;
    Persistent<Function> callback;
    // not used
    int error;
    string prompt;
    string passwd;
  };

  void DelayAsync(uv_work_t *req) {
    DelayBaton *baton = static_cast<DelayBaton*>(req->data);
    string prompt = baton->prompt;
    string passwd = getPassword(prompt);
    baton->passwd.assign(passwd);
  }

  void DelayAsyncAfter(uv_work_t *req, int status) {
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope handleScope(isolate);
    DelayBaton *baton = static_cast<DelayBaton*>(req->data);
    const int argc = 2;
    Local<Value> argv[] = {
      Undefined(isolate),
      Local<Value>(String::NewFromUtf8(isolate, baton->passwd.c_str()))
    };
    Local<Function>::New(isolate, baton->callback)->Call(isolate->GetCurrentContext()->Global(), argc, argv);
    baton->callback.Reset();
    delete baton;
  }

  void Delay(const FunctionCallbackInfo<Value>& args) {
    Isolate *isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    DelayBaton *baton = new DelayBaton;
    baton->request.data = (void*) baton;
    int cbIndex = 0;
    string prompt = "请输入密码";
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
    baton->prompt.assign(prompt, 0, prompt.length());
    Local<Function> cb = Local<Function>::Cast(args[cbIndex]);
    baton->callback.Reset(isolate, cb);
    baton->error = 0;
    uv_queue_work(uv_default_loop(), &baton->request, DelayAsync, DelayAsyncAfter);
  }

  void init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "getpass", Delay);
  }

  NODE_MODULE(getpass, init)
}

#endif
