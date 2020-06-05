/*

 ____   _    ____ _  ____  __    _    _   _
|  _ \ / \  / ___| |/ /  \/  |  / \  | \ | |
| |_) / _ \| |   | ' /| |\/| | / _ \ |  \| |
|  __/ ___ \ |___| . \| |  | |/ ___ \| |\  |
|_| /_/   \_\____|_|\_\_|  |_/_/   \_\_| \_|

g1953300 Okayama Kodai
Last Update 2020/6/6 Okayama Kodai

--------------------説明---------------------
パックマンを作りたかった。

パックマンをWASDもしくは方向キーで移動
画面上の黄色のアイテム（クッキー）を全て取るとクリア
敵にあたるとゲームオーバー
パワークッキー（少し大きめのクッキー）を食べると一定時間無敵
敵にあたるとスコアが増える
---------------------------------------------
*/

#include <handy.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

//ウィンドウサイズ
#define WINDOW 600.0
//盤面の大きさ
#define SIZE 25
//マスの大きさ
#define BOXSIZE WINDOW / SIZE
//キャラの半径
#define R BOXSIZE / 3
//プレイヤーの移動処理のウェイト
#define LOAD 500
//プレイヤーの無敵時間
#define POWER 15000
//描写処理のウェイト
#define WRITELOAD 100
//1回の処理でプレイヤーが移動する量
#define MOVE BOXSIZE / ((LOAD / WRITELOAD) - 1)

//自機情報
typedef struct My_Status
{
  //自機のマス上の座標
  int x;
  int y;
  //口の角度
  double mouse;
  //口を開くのか閉じるのか
  int mouse_switch;
  //動いているか(口が動く処理に使用)
  int move;
  //無敵時間の秒数をカウント -1で無敵ではない
  int power_count;
  //一度に連続で敵を倒した数
  int ren;
  //取ったアイテムの数
  int item_count;
  //スコア
  unsigned int score;
  //自機のウィンドウ上の座標
  double window_x;
  double window_y;
  //現在移動に使用しているキー
  unsigned int key;
  //次に移動するキー
  unsigned int nextkey;
  //無敵状態じゃない時に敵にぶつかったら1。死亡時のアニメーションに使用
  int dead;
  //敵を食べた時に立つフラグ
  int eated;
} My;

//敵の情報を保存するための構造体
typedef struct Enemy_Status
{
  int x;           //敵のマス上のx座標
  int y;           //敵のマス上のy座標
  double window_x; //敵のウィンドウ上のx座標
  double window_y; //敵のウィンドウ上のy座標
  int load;        //行動スピード
  int nervus;      //0で追跡　1で逃げる　2で食べられた
  int eaten;       //食べられた時に一瞬だけ自分を消す処理に使用するフラグ
} Enemy;

//幅優先探索をする際に座標のスタックをするための構造体
typedef struct BFS_Stack
{
  int x;      //x座標
  int y;      //y座標
  int before; //ひとつ前の配列のインデックス

} Stack;

//迷路を描写する
void DrawMaze(int board[SIZE][SIZE]);
//キャラクターを描写する
void Draw_Character(My *my_status_pointer, int board[SIZE][SIZE], Enemy *Enemy_list, doubleLayer *layers);
//スコアを表示する
void Draw_Right(My *my_status_pointer, doubleLayer *layers, int fps);
//オレンジの敵の動作
void Orange_Move(int board[SIZE][SIZE], My *my_status_pointer, Enemy *Enemy_list);
//プレイヤーの移動
void PlayerMove(My *my_status_pointer, int board[SIZE][SIZE]);
//ピンクの敵の動作
void Pink_Move(int board[SIZE][SIZE], My *my_status_pointer, Enemy *Enemy_list);
//赤の敵の動作
void Red_Move(int board[SIZE][SIZE], My *my_status_pointer, Enemy *Enemy_list);

//BFSの時にすでに同じところを調べていないかをチェックする関数
int BFS_Check(int x, int y, int add_count, Stack *Stack);
//敵からパックマンへの最短ルートを検索する
int BFS(int board[SIZE][SIZE], Enemy *enemy_status_pointer, int player_x, int player_y, int move_list[2]);
//青の敵の動作
int Blue_Move(int board[SIZE][SIZE], My *my_status_pointer, Enemy *Enemy_list);
//敵にぶつかった時の処理
int Crash(My *my_status_pointer, Enemy *Enemy_list, int board[SIZE][SIZE], doubleLayer layers, int fps);
//無敵時間時の敵の動作
int Enemy_Escape(int board[SIZE][SIZE], My *my_status_pointer, Enemy *enemy_status_pointer);
//敵が初期地点に帰る処理
int Enemy_Return(int board[SIZE][SIZE], My *my_status_pointer, Enemy *enemy_status_pointer);
//アイテムを取る処理
int Get_Item(My *my_status_pointer, int board[SIZE][SIZE], Enemy *Enemy_list, int item_number);
//迷路を生成する
int MakeMaze(int board[SIZE][SIZE]);
//パックマンが死んだ時のアニメーション
int Packman_Dead(My *my_status_pointer, doubleLayer *layers);

//度をラジアンにする
double DegreeToRadian(double degree);

//メイン関数
int main(void)
{
  //乱数シード初期化
  srand(time(NULL));

  //構造体宣言
  My MS;
  Enemy Enemy_list[4]; //0 red 1 pink  2 orange 3 blue

  //敵の構造体を初期化
  for (int i = 0; i < 4; i++)
  {
    Enemy_list[i].x = SIZE / 2;
    Enemy_list[i].y = SIZE / 2;
    Enemy_list[i].window_x = Enemy_list[i].x * BOXSIZE + BOXSIZE / 2;
    Enemy_list[i].window_y = (WINDOW - BOXSIZE) - Enemy_list[i].y * BOXSIZE + BOXSIZE / 2;
    Enemy_list[i].load = 1000;
    Enemy_list[i].nervus = 0;
    Enemy_list[i].eaten = 0;
  }

  //自分の構造体を初期化
  MS.x = SIZE / 2;
  MS.y = SIZE / 2 + 6;
  MS.window_x = MS.x * BOXSIZE + BOXSIZE / 2;
  MS.window_y = (WINDOW - BOXSIZE) - MS.y * BOXSIZE + BOXSIZE / 2;
  MS.mouse = 0.0001;
  MS.mouse_switch = 0;
  MS.move = 0;
  MS.ren = 0;
  MS.item_count = 0;
  MS.power_count = -1;
  MS.score = 0;
  MS.dead = 0;

  //構造体のポインタ
  My *my_status_pointer;
  my_status_pointer = &MS;

  //迷路初期化
  int board[SIZE][SIZE] = {0};

  //迷路作成
  int item_number = MakeMaze(board);

  //ウィンドウを開く
  //xの+200はスコアを表示する部分を後付けしています。
  HgOpen(WINDOW + 200, WINDOW);

  //キー入力を待つ
  HgSetFont(HG_G, 50);
  HgText(WINDOW / 2 - 80, WINDOW / 2 - 100, "Press Any Key");
  HgGetChar();
  HgClear();

  //スコアのレイヤ
  int lay = HgWAddLayer(0);

  //ダブルレイヤを設定
  doubleLayer layers = HgWAddDoubleLayer(0);

  //迷路描写
  DrawMaze(board);

  hgevent *event;
  HgSetEventMask(HG_KEY_DOWN);

  //wait用変数
  int wait = 0;
  //時間計測用変数
  clock_t start_t, end_t;
  int fps = 0;

  //最初のREADY!
  HgWSetFont(layers.display, HG_H, BOXSIZE * 0.8);
  HgWSetColor(layers.display, HG_YELLOW);
  HgWText(layers.display, WINDOW / 2 - BOXSIZE * 1.6, WINDOW / 2 - BOXSIZE * 3.5, "READY!");

  HgSleep(2);
  while (1)
  {
    //フレームリセット
    if (wait == 2001)
    {
      wait = 1;
    }

    //処理開始時間
    start_t = clock();
    //パックマンが死んでいないなら
    if (my_status_pointer->dead == 0)
    {
      event = HgEventNonBlocking();
      if (event != NULL)
      {
        //次に移動するキーを入れる
        my_status_pointer->nextkey = event->ch;
      }

      //処理を待ち順番が来たら実行する
      //プレイヤーの移動処理
      if (wait % LOAD == 0)
      {
        PlayerMove(my_status_pointer, board);

        if (Get_Item(my_status_pointer, board, Enemy_list, item_number) == 1)
        {
          Draw_Character(my_status_pointer, board, Enemy_list, &layers);
          HgSetFillColor(HgRGBA(115 / 255, 115 / 255, 115 / 255, 0.7));
          HgBoxFill(WINDOW / 4 - 7, WINDOW / 2, 300, 85, 0);
          HgSetFont(HG_HB, 70);
          HgSetColor(HG_YELLOW);
          HgText(WINDOW / 4, WINDOW / 2, "You Win");
          break;
        }
      }

      //敵の移動処理
      for (int i = 0; i < 4; i++)
      {
        if (wait % Enemy_list[i].load == 0)
        {
          if (Enemy_list[i].nervus == 0)
          {
            switch (i)
            {
            case 0:
              Red_Move(board, my_status_pointer, Enemy_list);
              break;
            case 1:
              Pink_Move(board, my_status_pointer, Enemy_list);
              break;
            case 2:
              Orange_Move(board, my_status_pointer, Enemy_list);
              break;
            case 3:
              Blue_Move(board, my_status_pointer, Enemy_list);
              break;
            }
          }
          else if (Enemy_list[i].nervus == 2) //変更しました
          {
            Enemy_Return(board, my_status_pointer, &Enemy_list[i]);
          }
          else
          {
            Enemy_Escape(board, my_status_pointer, &Enemy_list[i]);
          }
        }
      }

      //描写処理と衝突判定
      if (wait % WRITELOAD == 0)
      {
        //描写処理
        Draw_Character(my_status_pointer, board, Enemy_list, &layers);
        //スコア描写
        Draw_Right(my_status_pointer, &layers, fps);

        //衝突判定
        Crash(my_status_pointer, Enemy_list, board, layers, fps);
      }

      //無敵状態の時間制限を計測
      if (my_status_pointer->power_count != -1)
      {
        if (my_status_pointer->power_count > POWER)
        {
          for (int i = 0; i < 4; i++)
          {
            if (Enemy_list[i].nervus != 2)
            {

              Enemy_list[i].nervus = 0;
              Enemy_list[i].load = 1000;
              my_status_pointer->ren = 0;
            }
          }
        }
        else
        {
          my_status_pointer->power_count += 1;
        }
      }
      wait++;
    }
    else //パックマンが死んだときの処理
    {
      Draw_Right(my_status_pointer, &layers, fps);
      if (Packman_Dead(my_status_pointer, &layers) == 1)
      {
        HgSetFillColor(HgRGBA(115 / 255, 115 / 255, 115 / 255, 0.7));

        HgBoxFill(WINDOW / 4 - 7, WINDOW / 2, 340, 85, 0);
        HgSetFont(HG_HB, 70);
        HgSetColor(HG_BLUE);
        HgText(WINDOW / 4, WINDOW / 2, "You Lose");
        break;
      }
    }

    //処理終了時間
    end_t = clock();

    //フレームを合わせる

    // for (int i = 0; i < 100; i++)
    // {
    //   end_t = clock();
    // }

    while (end_t - start_t < 100)
    {
      end_t = clock();
    }
    fps = end_t - start_t;
  }
  HgSleep(1);
  HgGetChar();
  HgClose();
  return 0;
}

//度をラジアンにする
double DegreeToRadian(double degree)
{
  return (2 * M_PI) * (degree / 360);
}

//幅優先探索をする際にすでに同じところを調べていないかをチェックするための関数
//同じところなら0,新しいところなら1を返す
int BFS_Check(int x, int y, int add_count, Stack *Stack)
{
  for (int i = 0; i <= add_count; i++)
  {
    if (Stack[i].x == x && Stack[i].y == y)
    {
      return 0;
    }
  }
  return 1;
}

//幅優先探索
int BFS(int board[SIZE][SIZE], Enemy *enemy_status_pointer, int player_x, int player_y, int move_list[2])
{
  Stack Stack[SIZE * SIZE]; //スタックする構造体
  int add_count = 0;        //スタックに追加した数
  int search_count = 0;     //現在検索しているスタックのインデックス
  int x = 0;                //検索するx座標
  int y = 0;                //検索するy座標

  //初期座標を代入
  Stack[0].x = player_x; //初期x座標
  Stack[0].y = player_y; //初期y座標
  while (1)
  {

    x = Stack[search_count].x; //検索する対象のx座標
    y = Stack[search_count].y; //検索する対象のy座標

    //移動できるならスタックに入れる
    if (board[x - 1][y] != 1)
    {
      if (BFS_Check(x - 1, y, add_count, Stack))
      {
        add_count += 1;
        Stack[add_count].x = x - 1;
        Stack[add_count].y = y;
        Stack[add_count].before = search_count;
        if (Stack[add_count].x == enemy_status_pointer->x && Stack[add_count].y == enemy_status_pointer->y)
        {
          break;
        }
      }
    }
    if (board[x + 1][y] != 1)
    {
      if (BFS_Check(x + 1, y, add_count, Stack))
      {
        add_count += 1;
        Stack[add_count].x = x + 1;
        Stack[add_count].y = y;
        Stack[add_count].before = search_count;
        if (Stack[add_count].x == enemy_status_pointer->x && Stack[add_count].y == enemy_status_pointer->y)
        {
          break;
        }
      }
    }
    if (board[x][y - 1] != 1)
    {
      if (BFS_Check(x, y - 1, add_count, Stack))
      {
        add_count += 1;
        Stack[add_count].x = x;
        Stack[add_count].y = y - 1;
        Stack[add_count].before = search_count;
        if (Stack[add_count].x == enemy_status_pointer->x && Stack[add_count].y == enemy_status_pointer->y)
        {
          break;
        }
      }
    }
    if (board[x][y + 1] != 1)
    {
      if (BFS_Check(x, y + 1, add_count, Stack))
      {
        add_count += 1;
        Stack[add_count].x = x;
        Stack[add_count].y = y + 1;
        Stack[add_count].before = search_count;
        if (Stack[add_count].x == enemy_status_pointer->x && Stack[add_count].y == enemy_status_pointer->y)
        {
          break;
        }
      }
    }
    //到着不能の場合は1を返す
    if (search_count == add_count)
    {
      move_list[0] = enemy_status_pointer->x;
      move_list[1] = enemy_status_pointer->y;
      return 1;
    }
    search_count += 1;
  }

  //移動先のリストへ代入
  move_list[0] = Stack[Stack[add_count].before].x;
  move_list[1] = Stack[Stack[add_count].before].y;
  return 0;
}

//迷路を生成しアイテムを設置
int MakeMaze(int board[SIZE][SIZE])
{
  int item_number = 0; //生成したアイテムの数を記憶する変数
  //迷路を生成
  for (int y = 2; y < SIZE - 1; y++)
  {
    for (int x = 2; x < SIZE - 1; x++)
    {
      if (x % 2 == 0 && y % 2 == 0)
      {
        board[x][y] = 1;
        switch (rand() % 4)
        {
        case 0:
          board[x][y - 1] = 1;
          break;
        case 1:
          board[x + 1][y] = 1;
          break;
        case 2:
          board[x][y + 1] = 1;
          break;
        default:
          board[x - 1][y] = 1;
          break;
        }
      }
    }
  }
  //

  //外側の壁
  for (int i = 0; i < SIZE; i++)
  {
    board[0][i] = 1;
    board[SIZE - 1][i] = 1;
    board[i][0] = 1;
    board[i][SIZE - 1] = 1;
  }
  //

  //アイテム生成
  for (int i = 1; i < SIZE - 1; i++)
  {
    for (int j = 1; j < SIZE - 1; j++)
    {
      if (board[j][i] == 0)
      {
        if (board[j - 1][i] == 1 && board[j + 1][i] == 1 && board[j][i - 1] == 1 && board[j][i + 1] == 1)
        {
          continue;
        }
        else
        {
          if (rand() % 25 == 0)
          {
            board[j][i] = 3;
          }
          else
          {
            board[j][i] = 2;
          }
        }
      }
    }
  }
  //

  //敵のリスポーンポイント
  int boxsize = 2;
  int center = SIZE / 2;
  for (int i = 0; i <= boxsize * 2 + 2; i++)
  {
    for (int j = 0; j <= boxsize * 2 + 2; j++)
    {
      board[center - boxsize - 1 + j][center - boxsize - 1 + i] = 0;
    }
  }
  for (int i = 1; i <= boxsize; i++)
  {
    board[center - i][center - boxsize] = 1;
    board[center + i][center - boxsize] = 1;
  }
  //敵のリスポーン地点の入り口
  board[center][center - 2] = 4;

  for (int i = 0; i < boxsize * 2 + 1; i++)
  {
    board[center - boxsize + i][center + boxsize] = 1;
    board[center + boxsize][center - boxsize + i] = 1;
    board[center - boxsize][center - boxsize + i] = 1;
  }
  //

  //自機の初期座標ポイント
  for (int i = 0; i <= 3; i++)
  {
    for (int j = 0; j <= boxsize * 2 + 2; j++)
    {
      board[center - boxsize - 1 + j][center + boxsize + 1 + i] = 0;
    }
  }
  for (int j = 0; j <= boxsize * 2; j++)
  {
    board[center - boxsize + j][center + boxsize + 2] = 1;
  }
  board[center][center + boxsize + 3] = 1;

  for (int i = 1; i < SIZE - 1; i++)
  {
    for (int j = 1; j < SIZE - 1; j++)
    {
      if (board[j][i] == 2 || board[j][i] == 3)
      {
        item_number += 1;
      }
    }
  }

  //アイテムの数を返す
  return item_number;
}

//迷路とアイテムを描写する
void DrawMaze(int board[SIZE][SIZE])
{
  HgSetFillColor(HG_BLACK);
  HgBoxFill(0, 0, WINDOW, WINDOW, 1);
  for (int i = 0; i < SIZE; i++)
  {
    for (int j = 0; j < SIZE; j++)
    {
      if (board[j][i] == 1)
      {
        HgSetColor(HG_BLUE);
        HgSetWidth(2);
        HgBox(j * BOXSIZE, (WINDOW - BOXSIZE) - i * BOXSIZE, BOXSIZE, BOXSIZE);

        //壁がつながっているように見せるための処理
        //壁が隣接していたらHgLineで接しているところを上書きする
        HgSetColor(HG_BLACK);
        if (j != 0 && board[j - 1][i] == 1)
        {
          HgLine(j * BOXSIZE, (WINDOW - BOXSIZE) - i * BOXSIZE + 1, j * BOXSIZE, WINDOW - i * BOXSIZE - 1);
        }
        if (i != 0 && board[j][i - 1] == 1)
        {
          HgLine(j * BOXSIZE + 1, (WINDOW)-i * BOXSIZE, j * BOXSIZE + BOXSIZE - 1, (WINDOW)-i * BOXSIZE);
        }
      }

      //アイテムを描写する処理
      if (board[j][i] == 2)
      {
        HgSetWidth(1);
        HgSetFillColor(HG_YELLOW);
        HgCircleFill(j * BOXSIZE + BOXSIZE / 2, (WINDOW - BOXSIZE) - i * BOXSIZE + BOXSIZE / 2,
                     BOXSIZE / 7, 1);
      }
      else if (board[j][i] == 3)
      {
        HgSetWidth(1);
        HgSetFillColor(HG_YELLOW);
        HgCircleFill(j * BOXSIZE + BOXSIZE / 2, (WINDOW - BOXSIZE) - i * BOXSIZE + BOXSIZE / 2,
                     BOXSIZE / 4, 1);
      }
      else if (board[j][i] == 4)
      {
        HgSetWidth(2);
        HgSetColor(HG_YELLOW);
        HgLine(j * BOXSIZE, (WINDOW - BOXSIZE) - i * BOXSIZE + BOXSIZE, j * BOXSIZE + BOXSIZE, (WINDOW - BOXSIZE) - i * BOXSIZE + BOXSIZE);
      }
    }
  }
}

//自機移動-------------------------------------
void PlayerMove(My *my_status_pointer, int board[SIZE][SIZE])
{
  //自分のマス上の現在座標
  int my_x = my_status_pointer->x;
  int my_y = my_status_pointer->y;

  //次の入力キーが移動可能になったら移動方向を変える
  if (my_status_pointer->nextkey != 0)
  {
    switch (my_status_pointer->nextkey)
    {
    case 97:
    case HG_L_ARROW:
      if (board[my_x - 1][my_y] != 1 && board[my_x - 1][my_y] != 4)
      {
        my_status_pointer->key = 97;
        my_status_pointer->nextkey = 0;
      }
      break;
    case 100:
    case HG_R_ARROW:
      if (board[my_x + 1][my_y] != 1 && board[my_x + 1][my_y] != 4)
      {
        my_status_pointer->key = 100;
        my_status_pointer->nextkey = 0;
      }
      break;
    case 115:
    case HG_D_ARROW:
      if (board[my_x][my_y + 1] != 1 && board[my_x][my_y + 1] != 4)
      {
        my_status_pointer->key = 115;
        my_status_pointer->nextkey = 0;
      }
      break;
    case 119:
    case HG_U_ARROW:
      if (board[my_x][my_y - 1] != 1 && board[my_x][my_y - 1] != 4)
      {
        my_status_pointer->key = 119;
        my_status_pointer->nextkey = 0;
      }

      break;
    }
  }

  //移動する
  switch (my_status_pointer->key)
  {
  case 97:
    if (board[my_x - 1][my_y] != 1 && board[my_x - 1][my_y] != 4)
    {
      my_x -= 1;
      my_status_pointer->move = 1;
    }
    else
    {
      my_status_pointer->move = 0;
    }
    break;
  case 100:
    if (board[my_x + 1][my_y] != 1 && board[my_x + 1][my_y] != 4)
    {
      my_x += 1;
      my_status_pointer->move = 1;
    }
    else
    {
      my_status_pointer->move = 0;
    }
    break;
  case 115:
    if (board[my_x][my_y + 1] != 1 && board[my_x][my_y + 1] != 4)
    {
      my_y += 1;
      my_status_pointer->move = 1;
    }
    else
    {
      my_status_pointer->move = 0;
    }
    break;
  case 119:
    if (board[my_x][my_y - 1] != 1 && board[my_x][my_y - 1] != 4)
    {
      my_y -= 1;
      my_status_pointer->move = 1;
    }
    else
    {
      my_status_pointer->move = 0;
    }
    break;
  }
  my_status_pointer->x = my_x;
  my_status_pointer->y = my_y;
}

//パックマンが死ぬときのアニメーション
int Packman_Dead(My *my_status_pointer, doubleLayer *layers)
{

  int lay = HgLSwitch(layers);
  HgLClear(lay);
  HgWSetFillColor(lay, HG_YELLOW);
  //180度開くまで繰り返す
  if (my_status_pointer->mouse < M_PI - DegreeToRadian(1))
  {
    my_status_pointer->mouse += DegreeToRadian(0.5);
    HgWFanFill(lay, my_status_pointer->window_x, my_status_pointer->window_y, R, my_status_pointer->mouse + DegreeToRadian(90), -(my_status_pointer->mouse) + DegreeToRadian(90), 1);
    return 0;
  }
  else
  {
    return 1;
  }
  return -1;
}

//アイテムを取れるかの判定と全てのアイテムを取ったかの判定
int Get_Item(My *my_status_pointer, int board[SIZE][SIZE], Enemy *Enemy_list, int item_number)
{

  if (board[my_status_pointer->x][my_status_pointer->y] == 2) //クッキーを取る処理
  {
    board[my_status_pointer->x][my_status_pointer->y] = 0; //配列上でアイテムを消す
    my_status_pointer->item_count += 1;                    //アイテムのカウントを一つ増やす
    my_status_pointer->score += 10;                        //スコアを10増やす
    HgSetFillColor(HG_BLACK);
    HgCircleFill(my_status_pointer->x * BOXSIZE + BOXSIZE / 2, (WINDOW - BOXSIZE) - my_status_pointer->y * BOXSIZE + BOXSIZE / 2, R, 0); // 上から黒色を上書きしてアイテムを消す
  }
  else if (board[my_status_pointer->x][my_status_pointer->y] == 3) //パワークッキーを取る処理
  {
    for (int i = 0; i < 4; i++)
    {
      if (Enemy_list[i].nervus != 2)
      {
        Enemy_list[i].load = 2000;
        Enemy_list[i].nervus = 1;
      }
    }
    board[my_status_pointer->x][my_status_pointer->y] = 0;
    my_status_pointer->item_count += 1;
    my_status_pointer->score += 10;
    my_status_pointer->power_count = 0;
    HgSetFillColor(HG_BLACK);
    HgCircleFill(my_status_pointer->x * BOXSIZE + BOXSIZE / 2, (WINDOW - BOXSIZE) - my_status_pointer->y * BOXSIZE + BOXSIZE / 2, R, 0);
  }
  if (my_status_pointer->item_count == item_number)
  {
    return 1;
  }

  return 0;
}

//赤色の行動
//パックマンに最短距離でくる
void Red_Move(int board[SIZE][SIZE], My *my_status_pointer, Enemy *Enemy_list)
{
  int move_list[2] = {0};
  BFS(board, &Enemy_list[0], my_status_pointer->x, my_status_pointer->y, move_list);
  Enemy_list[0].x = move_list[0];
  Enemy_list[0].y = move_list[1];
}

//ピンク色の行動
//パックマンの移動している方向の3つ先(もしくはそれ以下)の位置に移動する
void Pink_Move(int board[SIZE][SIZE], My *my_status_pointer, Enemy *Enemy_list)
{
  int move_list[2] = {0};
  int x = my_status_pointer->x;
  int y = my_status_pointer->y;
  int key = my_status_pointer->key;
  int count = 0;
  //パックマンの移動している方向で3つ先の位置を求める
  switch (key)
  {
  case 97:
    while (1)
    {
      if (board[x - 1][y] != 1)
      {
        x -= 1;
        count += 1;
      }
      else if (count == 3)
      {
        break;
      }
      else
      {
        break;
      }
    }
    break;
  case 100:
    while (1)
    {
      if (board[x + 1][y] != 1)
      {
        x += 1;
        count += 1;
      }
      else if (count == 3)
      {
        break;
      }
      else
      {
        break;
      }
    }
    break;
  case 115:
    while (1)
    {
      if (board[x][y + 1] != 1)
      {
        y += 1;
        count += 1;
      }
      else if (count == 3)
      {
        break;
      }
      else
      {
        break;
      }
    }
    break;
  case 119:
    while (1)
    {
      if (board[x][y - 1] != 1)
      {
        y -= 1;
        count += 1;
      }
      else if (count == 3)
      {
        break;
      }
      else
      {
        break;
      }
    }
    break;
  }

  BFS(board, &Enemy_list[1], x, y, move_list);
  Enemy_list[1].x = move_list[0];
  Enemy_list[1].y = move_list[1];
}

//オレンジの行動
//ランダムな方向に移動する
void Orange_Move(int board[SIZE][SIZE], My *my_status_pointer, Enemy *Enemy_list)
{
  int x = Enemy_list[2].x;
  int y = Enemy_list[2].y;
  int flag = 0;
  while (1)
  {
    switch (rand() % 4)
    {
    case 0:
      if (board[x - 1][y] != 1)
      {
        x -= 1;
        flag = 1;
      }
      break;
    case 1:
      if (board[x + 1][y] != 1)
      {
        x += 1;
        flag = 1;
      }
      break;
    case 2:
      if (board[x][y - 1] != 1)
      {
        y -= 1;
        flag = 1;
      }
      break;
    case 3:
      if (board[x][y + 1] != 1)
      {
        y += 1;
        flag = 1;
      }
      break;
    }
    if (flag == 1)
    {
      break;
    }
  }
  Enemy_list[2].x = x;
  Enemy_list[2].y = y;
}

//青色の行動
//赤色とパックマンの点対称の位置に移動する
int Blue_Move(int board[SIZE][SIZE], My *my_status_pointer, Enemy *Enemy_list)
{
  int move_list[2] = {0};
  int my_x = my_status_pointer->x;
  int my_y = my_status_pointer->y;
  int red_x = Enemy_list[0].x;
  int red_y = Enemy_list[0].y;
  int point_x = 2 * my_x - red_x;
  int point_y = 2 * my_y - red_y;

  if (point_x < 1)
  {
    point_x = 1;
  }
  else if (point_x > SIZE - 1)
  {
    point_x = SIZE - 2;
  }

  if (point_y < 1)
  {
    point_y = 1;
  }
  else if (point_y > SIZE - 1)
  {
    point_y = SIZE - 2;
  }

  if (board[point_x][point_y] == 1)
  {
    if (board[point_x - 1][point_y] != 1)
    {
      point_x -= 1;
    }
    else if (board[point_x + 1][point_y] != 1)
    {
      point_x += 1;
    }
    else if (board[point_x][point_y + 1] != 1)
    {
      point_y += 1;
    }
    else if (board[point_x][point_y - 1] != 1)
    {
      point_y -= 1;
    }
    else
    {
      return 1;
    }
  }
  BFS(board, &Enemy_list[3], point_x, point_y, move_list);
  Enemy_list[3].x = move_list[0];
  Enemy_list[3].y = move_list[1];
  return 0;
}

//パックマンから逃げる処理
int Enemy_Escape(int board[SIZE][SIZE], My *my_status_pointer, Enemy *enemy_status_pointer)
{
  int move_list[2] = {0};
  int flag_count = 0;
  int e_x = enemy_status_pointer->x;
  int e_y = enemy_status_pointer->y;

  BFS(board, enemy_status_pointer, my_status_pointer->x, my_status_pointer->y, move_list);
  while (1)
  {
    switch (rand() % 4)
    {
    case 0:
      if (board[e_x - 1][e_y] != 1)
      {
        if (e_x - 1 != move_list[0] || e_y != move_list[1])
        {
          enemy_status_pointer->x = e_x - 1;
          enemy_status_pointer->y = e_y;
          flag_count += 10;
        }
        flag_count += 1;
      }
      break;
    case 1:
      if (board[e_x + 1][e_y] != 1)
      {
        if (e_x + 1 != move_list[0] || e_y != move_list[1])
        {
          enemy_status_pointer->x = e_x + 1;
          enemy_status_pointer->y = e_y;
          flag_count += 10;
        }
        flag_count += 1;
      }
      break;
    case 2:
      if (board[e_x][e_y - 1] != 1)
      {
        if (e_x != move_list[0] || e_y - 1 != move_list[1])
        {
          enemy_status_pointer->x = e_x;
          enemy_status_pointer->y = e_y - 1;
          flag_count += 10;
        }
        flag_count += 1;
      }
      break;
    case 3:
      if (board[e_x][e_y + 1] != 1)
      {
        if (e_x != move_list[0] || e_y + 1 != move_list[1])
        {
          enemy_status_pointer->x = e_x;
          enemy_status_pointer->y = e_y + 1;
          flag_count += 10;
        }
        flag_count += 1;
      }
      break;
    }
    if (flag_count >= 10)
    {
      break;
    }
  }
  return 0;
}

//敵が初期地点に戻る処理
int Enemy_Return(int board[SIZE][SIZE], My *my_status_pointer, Enemy *enemy_status_pointer)
{
  int move_list[2] = {0};
  if (BFS(board, enemy_status_pointer, SIZE / 2, SIZE / 2, move_list) == 1)
  {

    enemy_status_pointer->nervus = 0;
    enemy_status_pointer->load = 1000;
    enemy_status_pointer->x = SIZE / 2;
    enemy_status_pointer->y = SIZE / 2;
    enemy_status_pointer->window_x = enemy_status_pointer->x * BOXSIZE + BOXSIZE / 2;
    enemy_status_pointer->window_y = (WINDOW - BOXSIZE) - enemy_status_pointer->y * BOXSIZE + BOXSIZE / 2;

    return 1;
  }
  else
  {
    enemy_status_pointer->x = move_list[0];
    enemy_status_pointer->y = move_list[1];
  }
  return 0;
}

//敵との衝突判定
int Crash(My *my_status_pointer, Enemy *Enemy_list, int board[SIZE][SIZE], doubleLayer layers, int fps)
{
  double my_x = my_status_pointer->window_x;
  double my_y = my_status_pointer->window_y;
  double r = R - R / 2;
  for (int i = 0; i < 4; i++)
  {
    double e_x = Enemy_list[i].window_x;
    double e_y = Enemy_list[i].window_y;
    //敵と衝突した
    if (my_x + r >= e_x - r && my_x - r <= +e_x + r)
    {
      if (my_y + r >= e_y - r && my_y - r <= +e_y + r)
      {
        if (Enemy_list[i].nervus == 0)
        {
          HgSleep(0.2);
          my_status_pointer->mouse = 0;
          my_status_pointer->dead = 1;
        }
        else if (Enemy_list[i].nervus == 1)
        {
          //敵と衝突した時に得点をを表示させる
          Enemy_list[i].eaten = 1;
          //食べたフラグを立てる
          my_status_pointer->eated = 1;
          //連続で食べた数をカウント
          my_status_pointer->ren += 1;
          //得点。連続で敵を食べると上昇
          int score = (int)(pow(2, my_status_pointer->ren) * 100);

          Draw_Character(my_status_pointer, board, Enemy_list, &layers);
          Draw_Right(my_status_pointer, &layers, fps);
          HgWSetColor(layers.hidden, HG_SKYBLUE);
          HgWSetFont(layers.hidden, HG_HB, BOXSIZE / 1.6);
          HgWText(layers.hidden, Enemy_list[i].window_x - (BOXSIZE / 2), Enemy_list[i].window_y - (BOXSIZE / 4), "%d", score);
          Draw_Character(my_status_pointer, board, Enemy_list, &layers);
          HgSleep(0.4);
          Enemy_list[i].eaten = 0;
          my_status_pointer->eated = 0;
          my_status_pointer->score += score;
          Enemy_list[i].nervus = 2;
          Enemy_list[i].load = 500;
        }
      }
    }
  }
  return 0;
}
void Draw_Right(My *my_status_pointer, doubleLayer *layers, int fps)
{
  //スコア描写
  int lay = layers->hidden;
  int score = my_status_pointer->score;
  int count = 0;
  HgWSetFont(lay, HG_H, 30);
  while (score != 0) //文字を中央揃えをするためにスコアの桁数を計算
  {
    score /= 10;
    count += 1;
  }
  HgWText(lay, WINDOW + 45, WINDOW - 100, "SCORE");
  HgWText(lay, WINDOW + 92.5 - (count * 7), WINDOW - 200, "%d\n", my_status_pointer->score);
  HgWSetFont(lay, HG_H, 25);
  HgWText(lay, WINDOW + 10, WINDOW - 300, "%lu ミリ秒/処理", fps);
}

//キャラ描写
void Draw_Character(My *my_status_pointer, int board[SIZE][SIZE], Enemy *Enemy_list, doubleLayer *layers)
{
  //レイヤ切替
  int lay = HgLSwitch(layers);
  HgLClear(lay);
  HgWSetFillColor(lay, HG_YELLOW);

  double exact_window_x = 0;
  double exact_window_y = 0;
  //自分を描写
  if (my_status_pointer->eated == 0)
  {
    //移動先のウィンドウ座標
    exact_window_x = my_status_pointer->x * BOXSIZE + BOXSIZE / 2;
    exact_window_y = (WINDOW - BOXSIZE) - my_status_pointer->y * BOXSIZE + BOXSIZE / 2;

    //移動先になめらかに移動する
    if (my_status_pointer->window_x < exact_window_x - MOVE)
    {
      my_status_pointer->window_x += MOVE;
    }
    else if (my_status_pointer->window_x > exact_window_x + MOVE)
    {
      my_status_pointer->window_x -= MOVE;
    }
    else
    {
      my_status_pointer->window_x = exact_window_x;
    }

    if (my_status_pointer->window_y < exact_window_y - MOVE)
    {
      my_status_pointer->window_y += MOVE;
    }
    else if (my_status_pointer->window_y > exact_window_y + MOVE)
    {
      my_status_pointer->window_y -= MOVE;
    }
    else
    {
      my_status_pointer->window_y = exact_window_y;
    }

    //口を動かす処理
    if (my_status_pointer->move == 1)
    {

      if (my_status_pointer->mouse_switch == 0)
      {
        my_status_pointer->mouse += DegreeToRadian(25.0);
      }
      else
      {
        my_status_pointer->mouse -= DegreeToRadian(25.0);
      }

      if (my_status_pointer->mouse >= DegreeToRadian(75.0))
      {
        my_status_pointer->mouse_switch = 1;
      }
      else if (my_status_pointer->mouse <= DegreeToRadian(0.1))
      {
        my_status_pointer->mouse_switch = 0;
        my_status_pointer->mouse = DegreeToRadian(0.1);
      }
    }
    switch (my_status_pointer->key)
    {
    case 97:
      HgWFanFill(lay, my_status_pointer->window_x, my_status_pointer->window_y, R, my_status_pointer->mouse + DegreeToRadian(180.0), -(my_status_pointer->mouse) + DegreeToRadian(180.0), 1);
      break;
    case 115:
      HgWFanFill(lay, my_status_pointer->window_x, my_status_pointer->window_y, R, my_status_pointer->mouse + DegreeToRadian(270.0), -(my_status_pointer->mouse) + DegreeToRadian(270.0), 1);
      break;
    case 119:
      HgWFanFill(lay, my_status_pointer->window_x, my_status_pointer->window_y, R, my_status_pointer->mouse + DegreeToRadian(90.0), -(my_status_pointer->mouse) + DegreeToRadian(90.0), 1);
      break;
    default:
      HgWFanFill(lay, my_status_pointer->window_x, my_status_pointer->window_y, R, my_status_pointer->mouse, -(my_status_pointer->mouse), 1);
      break;
    }
  }

  //敵を描写
  for (int i = 0; i < 4; i++)
  {
    if (Enemy_list[i].eaten == 0)
    {
      if (Enemy_list[i].nervus == 0)
      {
        switch (i)
        {
        case 0:
          HgWSetFillColor(lay, HG_RED);
          break;
        case 1:
          HgWSetFillColor(lay, HG_PINK);
          break;
        case 2:
          HgWSetFillColor(lay, HG_ORANGE);
          break;
        case 3:
          HgWSetFillColor(lay, HG_SKYBLUE);
          break;
        }
      }
      else if (Enemy_list[i].nervus == 1 && my_status_pointer->power_count >= POWER - POWER / 3 && my_status_pointer->power_count % 1000 == 0)
      {
        HgWSetFillColor(lay, HG_BLACK);
        HgWSetColor(lay, HG_WHITE);
      }
      else if (Enemy_list[i].nervus == 1)
      {
        HgWSetFillColor(lay, HG_BLUE);
      }
      else
      {
        HgWSetFillColor(lay, HG_BLACK);
        HgWSetColor(lay, HG_WHITE);
      }

      //敵の移動先の正確なウィンドウ座標
      exact_window_x = Enemy_list[i].x * BOXSIZE + BOXSIZE / 2;
      exact_window_y = (WINDOW - BOXSIZE) - Enemy_list[i].y * BOXSIZE + BOXSIZE / 2;

      //一回の処理で敵が移動する距離
      double enemy_move = BOXSIZE / ((Enemy_list[i].load / WRITELOAD));

      if (Enemy_list[i].window_x < exact_window_x - enemy_move)
      {
        Enemy_list[i].window_x += enemy_move;
      }
      else if (Enemy_list[i].window_x > exact_window_x + enemy_move)
      {
        Enemy_list[i].window_x -= enemy_move;
      }
      else
      {
        Enemy_list[i].window_x = exact_window_x;
      }

      if (Enemy_list[i].window_y < exact_window_y - enemy_move)
      {
        Enemy_list[i].window_y += enemy_move;
      }
      else if (Enemy_list[i].window_y > exact_window_y + enemy_move)
      {
        Enemy_list[i].window_y -= enemy_move;
      }
      else
      {
        Enemy_list[i].window_y = exact_window_y;
      }
      HgWCircleFill(lay, Enemy_list[i].window_x, Enemy_list[i].window_y, R, 1);
      HgWSetColor(lay, HG_BLACK);
    }
  }
}