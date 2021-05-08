// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This module implements pthread API on top of FreeRTOS. API is implemented to the level allowing
// libstdcxx threading framework to operate correctly. So not all original pthread routines are supported.
//

#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "esp_err.h"
#include "esp_attr.h"
#include "rom/queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ahu_utils.h"

#define LOG_LOCAL_LEVEL CONFIG_LOG_DEFAULT_LEVEL
void Data_transfer(double array1[],const double array2[],int size1,int size2)
{
    for(int i = size1; i < size2; i++){
        array1[i-size1] = array2[i];
    }
}
void sort_descend(double array[],int size) //闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊瑜夐弸鏍煛閸ャ儱鐏╃紒鎰殜閺岀喖鏌囬敃鈧弸鐔兼煛閸℃鐭掗柟顔筋殔閳藉鈻嶉搹顐㈢仼缂佽京鍋ゅ畷鍫曨敆娴ｅ搫骞楅梻渚€鈧稑宓嗘繛浣冲嫭娅犳い鏂款潟娴滄粓鐓崶銊﹀暗闁糕晪缍侀弻鐔碱敊鐟欐帒婀辩划璇测槈閵忕姷顔掗梺鍛婃尫缁讹繝宕€ｎ喗鈷掑〒姘ｅ亾闁逞屽墰閸嬫盯鎳熼娑欐珷妞ゆ柨顫曟禍婊堟煥閺冨倸浜鹃柡鍡╁墰閳ь剚顔栭崳顕€宕戞繝鍥╁祦婵☆垰鍚嬬€氭岸鏌涘▎蹇ｆ▓婵☆偆鍠栧缁樼瑹閳ь剙顭囪閹囧幢濡炪垺绋戣灃闁告粈鐒﹂弲婊堟⒑閸撴彃浜濇繛鍙夛耿閸╂盯骞嬮敂钘変化闂佽鍘界敮鎺撲繆婵傚憡鐓涢悗锝庡亜閻忔挳鏌″畝瀣？闁逞屽墾缂嶅棙绂嶉崼鏇熷亗闁稿繒鈷堝▓浠嬫煟閹邦垰鐨虹紒鐘差煼閺岀喖顢欓悾宀€鐓夐梺鐟扮－閸嬨倖淇婇悜鑺ユ櫆缂佹稑顑勯幋鐑芥⒒閸屾艾鈧绮堟笟鈧獮鏍敃閿曗偓绾惧綊鏌涢锝嗙缁炬儳缍婇弻鈥愁吋鎼粹€茬爱闂佺ǹ顑嗛幐鎼侊綖濠靛鏁嗛柛灞剧敖閵娾晜鈷戦柛婵嗗椤箓鏌涢弮鈧崹鍧楃嵁閸愵喖顫呴柕鍫濇噹缁愭稒绻濋悽闈浶㈤悗姘间簽濡叉劙寮撮姀鈾€鎷绘繛杈剧到閹芥粎绮旈悜妯镐簻闁靛闄勫畷宀€鈧娲橀〃鍛达綖濠婂牆鐒垫い鎺嗗亾妞ゆ洩缍侀、鏇㈡晝閳ь剛绮绘繝姘仯闁搞儜鍐獓濡炪們鍎茬换鍫濐潖濞差亝顥堟繛鎴炶壘椤ｅ搫鈹戦埥鍡椾簼妞ゃ劌锕妴渚€寮崼鐔告闂佽法鍣﹂幏锟�
{
    double temp;//闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊瑜夐弸鏍煛閸ャ儱鐏╃紒鎰殜閺岀喖鎮ч崼鐔哄嚒闂佸憡鍨规慨鎾煘閹达附鍋愰悗鍦Т椤ユ繄绱撴担鍝勵€岄柛銊ㄦ硾椤繐煤椤忓嫬鐎銈嗘礀閹冲酣宕滈崡鐑嗘富闁靛浂鍨粈浣该洪敃鍌氱厱闁硅揪闄勯崑锝夋煙椤撶喎绗掑┑鈥炽偢閺岋紕鈧綆鍋勯悘鎾煛瀹€瀣？闁逞屽墾缂嶅棙绂嶉崼鏇熷亗闁稿繒鈷堝▓浠嬫煟閹邦垰鐨虹紒鐘差煼閺岀喖顢欓悾宀€鐓夐梺鐟扮－閸嬨倖淇婇悜鑺ユ櫆缂佹稑顑勯幋鐑芥⒒閸屾艾鈧绮堟笟鈧獮鏍敃閿曗偓绾惧綊鏌涢锝嗙缁炬儳缍婇弻鈥愁吋鎼粹€茬爱闂佺ǹ顑嗛幐鎼侊綖濠靛鍊锋い鎺嗗亾妞ゅ骏鎷�
    for(int i = 0; i < size-1; i++)
    {
        for(int j = size-1; j > i; j--)
        {
            if(array[j] > array[j-1]){
                temp = array[j];
                array[j] = array[j-1];
                array[j-1] = temp;
            }
        }
    }
}
void sort_ascend(double array[],int size) //闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊瑜夐弸鏍煛閸ャ儱鐏╃紒鎰殜閺岀喖鎮ч崼鐔哄嚒闂佸憡鍨规慨鎾煘閹达附鍋愰悗鍦Т椤ユ繄绱撴担鍝勵€岄柛銊﹀▕閸┾偓妞ゆ帒鍊归崵鈧柣搴㈠嚬閸樼晫绮╅悢鐓庡耿婵＄偑鍎茬敮鈥崇暦婵傜ǹ唯闁挎棁顫夌€氬ジ姊洪懡銈呅ｅù婊€绮欏畷婵嗙暆閸曨偄鍤戦梺纭呮彧闂勫嫰鎮″☉銏″€甸柨婵嗙凹閹查箖鏌ｉ幘鍐叉倯妞ゃ劊鍎甸幃娆撴嚑椤掑偆鍟嬮梻浣筋嚃閸犳鍒掗幘璇茬疇闁绘劕鎼敮闂佹寧姊婚悺鏃€绂掕濮婂宕掑▎鎴М闂佸湱鈷堥崑濠傜暦閹版澘閱囬柡鍥╁仧閿涙瑩姊洪崫鍕枆闁稿鍋撻梺绋款儐閹告悂锝炲┑瀣垫晢闁稿本绨介妸鈺傗拺闁告繂瀚€氫即鏌熼崘鑼妞ゆ洏鍎靛畷鐔碱敍濮樿京鏉告俊鐐€栭幐楣冨窗閹伴偊鏁傞柣鏂垮悑閳锋帒霉閿濆懏鍟為柟顖氱墦閺屾稒绻濋崒婊冪厽閻庤娲橀崝娆忣嚕娴犲鏁冮柣鏃囨腹婢规洖鈹戦鏂や緵闁告挻绋撶划鏂棵洪鍛幐闁诲繒鍋犻褎鎱ㄩ崒鐐寸厵妞ゆ柨鎼悘鑼偓瑙勬磸閸斿秶鎹㈠┑瀣妞ゅ繐妫涢敍鐔兼⒒閸屾艾鈧绮堟担铏圭濠电姴娉氭径鎰閻犲洩灏欓ˇ褍鈹戦濮愪粶闁稿鎸鹃埀顒侇問閸ｎ噣宕戞繝鍥╁祦婵☆垰鍚嬬€氭岸鏌涘▎蹇ｆ▓婵☆偆鍠栧缁樼瑹閳ь剙顭囪閹囧幢濡炪垺绋戣灃闁告粈鐒﹂弲婊堟⒑閸撴彃浜濇繛鍙夛耿閸╂盯骞嬮敂钘変化闂佽鍘界敮鎺撲繆婵傚憡鐓涢悗锝庡亜閻忔挳鏌″畝瀣？闁逞屽墾缂嶅棙绂嶉崼鏇熷亗闁稿繒鈷堝▓浠嬫煟閹邦垰鐨虹紒鐘差煼閺岀喖顢欓悾宀€鐓夐梺鐟扮－閸嬨倖淇婇悜鑺ユ櫆缂佹稑顑勯幋鐑芥⒒閸屾艾鈧绮堟笟鈧獮鏍敃閿曗偓绾惧綊鏌涢锝嗙缁炬儳缍婇弻鈥愁吋鎼粹€茬爱闂佺ǹ顑嗛幐鎼侊綖濠靛鍊锋い鎺嗗亾妞ゅ骏鎷�
{
    double temp;//闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊瑜夐弸鏍煛閸ャ儱鐏╃紒鎰殜閺岀喖鎮ч崼鐔哄嚒闂佸憡鍨规慨鎾煘閹达附鍋愰悗鍦Т椤ユ繄绱撴担鍝勵€岄柛銊ㄦ硾椤繐煤椤忓嫬鐎銈嗘礀閹冲酣宕滈崡鐑嗘富闁靛浂鍨粈浣该洪敃鍌氱厱闁硅揪闄勯崑锝夋煙椤撶喎绗掑┑鈥炽偢閺岋紕鈧綆鍋勯悘鎾煛瀹€瀣？闁逞屽墾缂嶅棙绂嶉崼鏇熷亗闁稿繒鈷堝▓浠嬫煟閹邦垰鐨虹紒鐘差煼閺岀喖顢欓悾宀€鐓夐梺鐟扮－閸嬨倖淇婇悜鑺ユ櫆缂佹稑顑勯幋鐑芥⒒閸屾艾鈧绮堟笟鈧獮鏍敃閿曗偓绾惧綊鏌涢锝嗙缁炬儳缍婇弻鈥愁吋鎼粹€茬爱闂佺ǹ顑嗛幐鎼侊綖濠靛鍊锋い鎺嗗亾妞ゅ骏鎷�
    for(int i = 0; i < size-1; i++)
    {
        for(int j = size-1; j > i; j--)
        {
            if(array[j] < array[j-1]){
                temp = array[j];
                array[j] = array[j-1];
                array[j-1] = temp;
            }
        }
    }
}
void sort(int array[],int size)
{
    int temp;//闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊瑜夐弸鏍煛閸ャ儱鐏╃紒鎰殜閺岀喖鎮ч崼鐔哄嚒闂佸憡鍨规慨鎾煘閹达附鍋愰悗鍦Т椤ユ繄绱撴担鍝勵€岄柛銊ㄦ硾椤繐煤椤忓嫬鐎銈嗘礀閹冲酣宕滈崡鐑嗘富闁靛浂鍨粈浣该洪敃鍌氱厱闁硅揪闄勯崑锝夋煙椤撶喎绗掑┑鈥炽偢閺岋紕鈧綆鍋勯悘鎾煛瀹€瀣？闁逞屽墾缂嶅棙绂嶉崼鏇熷亗闁稿繒鈷堝▓浠嬫煟閹邦垰鐨虹紒鐘差煼閺岀喖顢欓悾宀€鐓夐梺鐟扮－閸嬨倖淇婇悜鑺ユ櫆缂佹稑顑勯幋鐑芥⒒閸屾艾鈧绮堟笟鈧獮鏍敃閿曗偓绾惧綊鏌涢锝嗙缁炬儳缍婇弻鈥愁吋鎼粹€茬爱闂佺ǹ顑嗛幐鎼侊綖濠靛鍊锋い鎺嗗亾妞ゅ骏鎷�
    for(int i = 0; i < size-1; i++)
    {
        for(int j = size-1; j > i; j--)
        {
            if(array[j] < array[j-1]){
                temp = array[j];
                array[j] = array[j-1];
                array[j-1] = temp;
            }
        }
    }
}
int ismember(const int array[],const int size,int e)
{
    for(int i = 0; i < size; i++){
        if(array[i] == e)
            return 1;
    }
    return 0;
}
double mean(const double array[],int length1, int length2)
{
    double sum = 0;
    for(int i = length1; i < length2; i++)
    {
        sum += array[i];
    }
    double m = sum/(length2-length1);
    return m;
}
double standard_deviation(const double array[],int len1,int len2){
    double avg = mean(array,len1,len2);//濠电姷鏁告慨鐢割敊閺嶎厼绐楁慨妯挎硾缁犵娀鏌熼幑鎰靛殭缁炬儳缍婇弻鐔兼⒒鐎电ǹ濡介梺缁樻尪閸ㄤ粙寮诲澶婁紶闁告洦鍓欏▍锝夋⒒娴ｆ枻楠忛柛濠冪箞瀵鈽夐姀鐘栥劑鏌熺€涙ɑ鐓ュù婊堢畺濮婅櫣绮欏▎鎯у壉闂佸湱鎳撳ú顓烆嚕椤愶箑绠荤紓浣股戝▍銏ゆ⒑鐠恒劌娅愰柟鍑ゆ嫹
    double sum = 0.;
    for(int i = len1; i < len2; i++){
        sum += (array[i] - avg) * (array[i] - avg);
    }
    return sqrt(sum/(len2-len1));
}
void max_v_interval(const double array[],double maxValueIdex[],int size)
{
    double max_value = 0;
    int max_index = 0;
    for(int i = 0; i < size; i++)
    {
        if(max_value < array[i]){
            max_value = array[i];
            max_index = i;
        }
    }
    maxValueIdex[0] = max_value;
    maxValueIdex[1] = max_index + 1;
}

void max_v_interval_fabs(const double array[],double maxValueIdex[],int size)
{
    double max_value = 0;
    int max_index = 0;
    for(int i = 0; i < size; i++)
    {
        if(max_value < fabs(array[i])){
            max_value = fabs(array[i]);
            max_index = i;
        }
    }
    maxValueIdex[0] = max_value;
    maxValueIdex[1] = max_index;
}

void max_v_spec(const double array[],double maxValueIdex[],int size)
{
    double * temp = (double *)malloc(size*sizeof(double));
    for(int i = 0; i < size; i++)
    {
        temp[i] = array[i+1];
    }
    double max_value = 0;
    int max_index = 0;
    for(int i = 0; i < size; i++)
    {
        if(max_value < temp[i]){
            max_value = temp[i];
            max_index = i;
        }
    }
    maxValueIdex[0] = max_value;
    maxValueIdex[1] = max_index + 1;
    free(temp);temp = NULL;
}
double onearray_sum(double array[],int size)
{
    double sum = 0;
    for(int i = 0; i < size; i++){
        sum = sum + array[i];
    }
    return sum;
}
double onearray_sum_fabs(double array[],int size)
{
    double sum = 0;
    for(int i = 0; i < size; i++){
        sum = sum + fabs(array[i]);
    }
    sum=sum/size;
    return sum;
}
//matlab闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊瑜夐弸鏍煛閸ャ儱鐏╃紒鎰殜閺岀喖鎮ч崼鐔哄嚒闂佸憡鍨规慨鎾煘閹达附鍋愰悗鍦Т椤ユ繄绱撴担鍝勵€岄柛銊ョ－濡叉劙骞掗弬鐐媰缂備焦绋戦鍛村箖閻氼柆闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊瑜夐弸鏍煛閸ャ儱鐏╃紒鎰殜閺岀喖鎮ч崼鐔哄嚒闂佸憡鍨规慨鎾煘閹达附鍋愰悗鍦Т椤ユ繄绱撴担鍝勵€岄柛銊ョ埣瀵鏁愭径濠勵吅闂佹寧绻傞幉娑㈠箻缂佹鍘搁梺鍛婁緱閸犳宕愰幇鐗堢厸鐎光偓鐎ｎ剛鐦堥悗瑙勬礃鐢帟鐏掗柣鐐寸▓閳ь剙鍘栨竟鏇㈡⒑閸濆嫮鈻夐柛瀣у亾闂佺ǹ顑嗛幐鎼侊綖濠靛鏁嗛柛灞剧敖閵娾晜鈷戦柛婵嗗椤箓鏌涢弮鈧崹鍧楃嵁閸愵喖顫呴柕鍫濇噹缁愭稒绻濋悽闈浶㈤悗姘间簽濡叉劙寮撮姀鈾€鎷绘繛杈剧悼閸庛倝宕甸埀顒勬⒑閹肩偛鈧牠鎮ч悩鑽ゅ祦闊洦绋掗弲鎼佹煥閻曞倹瀚�
void diff(double *dat,int datalen,double*ret_buf,int *ret_len)
{
 	int i,j=0;
  for(i = 1; i<datalen; i++)
  {
    ret_buf[j++] = dat[i]-dat[i-1];
  }
  *ret_len = j;
}

void diff_fabs(double *dat,int datalen,double*ret_buf,int *ret_len)
{
 	int i,j=0;
  for(i = 1; i<datalen; i++)
  {
    ret_buf[j++] = fabs(dat[i]-dat[i-1]);//婵犵數濮甸鏍窗濮樿泛绠犻柟鎹愵嚙绾惧綊鏌涢敂璇插箺閻庢碍宀搁弻锝夊箛椤旂厧濡洪梺绋匡功閸忔﹢骞冨畡鎵虫瀻闊洦鎼╂禒鎯р攽閻愯尙澧曞褍娴峰Σ鎰板箻鐎涙ê顎撻梺闈╁瘜閸橀箖鎮鹃棃娑辨富闁靛牆妫欐径鍕煕閿濆繒鍒伴柣锝囧厴楠炲洭鎮ч崼鐔割仧闂備浇娉曢崳锕傚箯閿燂拷
  }
  *ret_len = j;
}

double accumulate1(const double array[], int size,int seg)
{
    double * mul = (double *)malloc(size*sizeof(double));
    double * mul_divide = (double *)malloc(size*sizeof(double));
    for(int i = 0; i < size; i++){
        mul[i] = array[i]*array[i];
    }
    for(int i = 0; i < size; i++){
        mul_divide[i] = mul[i]/seg;
    }
    double sum = onearray_sum(mul_divide,size);
    double energy_vec = sqrt(sum);
    free(mul); mul = NULL;
    free(mul_divide);mul_divide = NULL;
    return energy_vec;
}
double accumulate2(const double array1[], const double array2[],int size)
{
    double * mul1 = (double *)malloc(size*sizeof(double));
    double * mul2 = (double *)malloc(size*sizeof(double));
    double * mul3 = (double *)malloc(size*sizeof(double));
    for(int i = 0; i < size; i++){
        mul1[i] = array1[i]*array2[i];
    }
    for(int i = 0; i < size; i++){
        mul2[i] = array1[i]*array1[i];
    }
    for(int i = 0; i < size; i++){
        mul3[i] = array2[i]*array2[i];
    }
    double sum1 = onearray_sum(mul1,size);
    double sum2 = onearray_sum(mul2,size);
    double sum3 = onearray_sum(mul3,size);

    double result = sum1/sqrt(sum2*sum3);
    free(mul1);mul1 = NULL;
    free(mul2);mul2 = NULL;
    free(mul3);mul3 = NULL;
    return result;
}

/** 闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊瑜夐弸鏍煛閸ャ儱鐏╃紒鎰殜閺岀喖鎮ч崼鐔哄嚒闂佸憡鍨规慨鎾煘閹达附鍋愰悗鍦Т椤ユ繄绱撴担鍝勵€岄柛銊ョ埣瀵鏁愭径濠勵吅闂佹寧绻傞幉娑㈠箻缂佹鍘搁梺鍛婁緱閸犳宕愰幇鐗堢厸鐎光偓鐎ｎ剛鐦堥悗瑙勬礃鐢帟鐏掗柣鐐寸▓閳ь剙鍘栨竟鏇㈡⒑閸濆嫮鈻夐柛瀣у亾闂佺ǹ顑嗛幐鎼侊綖濠靛鏁嗛柛灞剧敖閵娾晜鈷戦柛婵嗗椤箓鏌涢弮鈧崹鍧楃嵁閸愵喖顫呴柕鍫濇閳ь剛绮穱濠囶敍濠靛浂浠╁銈嗘煥濡绢湩kdet闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊瑜夐弸鏍煛閸ャ儱鐏╃紒鎰殜閺岀喖鎮ч崼鐔哄嚒闂佸憡鍨规慨鎾煘閹达附鍋愰悗鍦Т椤ユ繄绱撴担鍝勵€岄柛銊ョ埣瀵鏁愭径濠勵吅闂佹寧绻傞幉娑㈠箻缂佹鍘搁梺鍛婁緱閸犳宕愰幇鐗堢厸鐎光偓鐎ｎ剛鐦堥悗瑙勬礃鐢帟鐏掗柣鐐寸▓閳ь剙鍘栨竟鏇㈡⒑閸濆嫮鈻夐柛瀣у亾闂佺ǹ顑嗛幐鎼侊綖濠靛鏁嗛柛灞剧敖閵娾晜鈷戦柛婵嗗椤箓鏌涢弮鈧崹鍧楃嵁閸愵喖顫呴柕鍫濇噹缁愭稒绻濋悽闈浶㈤悗姘间簽濡叉劙寮撮姀鈾€鎷绘繛杈剧悼閸庛倝宕甸埀顒勬⒑閹肩偛鐏柣鎾偓宕囨殾闁哄被鍎遍拑鐔兼煏婢跺牆鍔ら柛鏃撶畱椤啴濡堕崱妤€娼戦梺绋款儐閹瑰洭寮婚弴銏犻唶闁绘梻枪閳ь剚鍔楅埀顒侇問閸ｎ噣宕戞繝鍥╁祦婵☆垰鍚嬬€氭岸鏌涘▎蹇ｆ▓婵☆偆鍠栧缁樼瑹閳ь剙顭囪閹囧幢濡炪垺绋戣灃闁告粈鐒﹂弲婊堟⒑閼测敚褰掓倶濮樿泛鐭楅煫鍥ㄧ⊕閻撱儲绻濋棃娑欘棡闁革絿鏅槐鎺楁偐瀹割喚鍚嬮梺鍝勫閸撴繈骞忛崨顖涘枂闁告洦鍋嗛敍鎾绘煟鎼淬埄鍟忛柛锝庡櫍瀹曟粓鎮㈤梹鎰畾闂佸壊鍋呭ú鏍嵁閵忊€茬箚闁靛牆鎷戝妤冪磼閹插鐣垫慨濠傤煼瀹曟帒鈻庨幇顔哄仒婵犵數鍋涢ˇ鏉棵洪悢椋庢殾濞村吋娼欑粻娑㈡煛婢跺﹦浠㈤柛銈嗗灦缁绘繈鎮介棃娴讹綁鏌ら悷鏉库挃缂侇喖顭烽、娑樷攦閹傚闂佽崵鍠愬姗€顢旈鍕电唵闁荤喖鍋婇崕鏃傗偓瑙勬礃閸ㄥ潡鐛Ο灏栧亾閸偅鍊愭俊缁㈠櫍濮婃椽宕ㄦ繝鍕ㄦ闂佹寧娲忛崕閬嶅煝瀹ュ顫呴柕鍫濇閹锋椽姊洪崨濠勭畵閻庢凹鍓熼崺鈧い鎺嗗亾闁绘牜鍘ч悾鐑藉箛閺夊灝鍞ㄥ銈嗗姉閸犲孩绂嶉悙顒佸弿婵☆垵顕ф晶顖涚箾閸忕厧濮嶉柡宀€鍠栭、娑樷槈濞嗘ɑ顥堥柣搴㈩問閸ｎ噣宕抽敐鍛殾濠靛倸鎲￠崑鍕煕濞戞ǚ鐪嬪ù鐘欏洦鈷掗柛灞剧懅椤︼箓鏌熺喊鍗炰喊鐎规洘鍔欏畷濂稿即閻愮绱梻浣告惈缁嬩線宕戦埀顒勬煕鐎ｎ偅灏い顐ｇ箞椤㈡宕掑┃鐐姂濮婃椽宕崟顕呮蕉闂佸憡姊归崹鍧楃嵁閸愵喖顫呴柕鍫濇噹缁愭稒绻濋悽闈浶㈤悗姘间簽濡叉劙寮撮姀鈾€鎷绘繛杈剧到閹芥粎绮旈悜妯镐簻闁靛闄勫畷宀€鈧娲橀〃鍛达綖濠婂牆鐒垫い鎺嗗亾妞ゆ洩缍侀、鏇㈡晝閳ь剛绮绘繝姘仯闁搞儜鍐獓濡炪們鍎茬换鍫濐潖濞差亝顥堟繛鎴炶壘椤ｅ搫鈹戦埥鍡椾簼妞ゃ劌锕妴渚€寮崼婵嬪敹闂佸搫娲ㄩ崯鍧楀箯濞差亝鐓熼柣妯哄帠閼割亪鏌涢弬璺ㄧ劯鐎殿喚枪閳诲酣骞囬埡鈧花璇差渻閵堝棗濮х紓宥呮閳绘挸顫滈埀顒勫蓟閳╁啰鐟归柛銉戝嫮褰庢俊鐐€戦崹娲偡閳哄懎绠板┑鐘插暙缁剁偞淇婇姘础濠㈣泛鍨婚梻鍌氬€搁崐鎼佸磹瀹勬噴褰掑炊瑜夐弸鏍煛閸ャ儱鐏╃紒鎰殜閺岀喖鎮ч崼鐔哄嚒闂佸憡鍨规慨鎾煘閹达附鍋愰悗鍦Т椤ユ繄绱撴担鍝勵€岄柛銊ョ埣瀵鏁愭径濠勵吅闂佹寧绻傞幉娑㈠箻缂佹鍘搁梺鍛婁緱閸犳宕愰幇鐗堢厸鐎光偓鐎ｎ剛鐦堥悗瑙勬礃鐢帟鐏掗柣鐐寸▓閳ь剙鍘栨竟鏇㈡⒑閸濆嫮鈻夐柛瀣у亾闂佺ǹ顑嗛幐鎼侊綖濠靛鏁嗛柛灞剧敖閵娾晜鈷戦柛婵嗗椤箓鏌涢弮鈧崹鍧楃嵁閸愵喖顫呴柕鍫濇濞堛儵姊洪棃娑氬婵炲眰鍔岄悾宄懊洪鍛嫼濠殿喚鎳撳ú銈夋倿濞差亝鐓曢柕濞垮劘閸嬨垻鈧娲樺妯跨亙闂佸憡鍔х徊鑺ョ閻愵剚鍙忔慨妤€妫楁晶鎵磼婢跺苯鍔嬫い銊ｅ劦閹瑩骞栭鐔烘殽闂佺粯鎸堕崐鏍Φ閸曨喚鐤€闁圭偓娼欏▍銈呪攽閻愬弶鍣烽柛銊ㄦ椤繐煤椤忓嫪绱堕梺鍛婃处閸嬧偓闁稿鎹囧畷濂稿即閻愮绱梻浣告惈缁嬩線宕戦埀顒勬煕鐎ｎ偅灏い顐ｇ箞椤㈡宕掑┃鐐姂濮婃椽宕崟顕呮蕉闂佸憡姊归崹鍧楃嵁閸愵喖顫呴柕鍫濇噹缁愭稒绻濋悽闈浶㈤悗姘间簽濡叉劙寮撮姀鈾€鎷绘繛杈剧到閹芥粎绮旈悜妯镐簻闁靛闄勫畷宀€鈧娲橀〃鍛达綖濠婂牆鐒垫い鎺嗗亾妞ゆ洩缍侀、鏇㈡晝閳ь剛绮绘繝姘仯闁搞儜鍐獓濡炪們鍎茬换鍫濐潖濞差亝顥堟繛鎴炶壘椤ｅ搫鈹戦埥鍡椾簼妞ゃ劌锕妴渚€寮崼婵嬪敹闂佸搫娲ㄩ崰搴㈢椤撱垺鈷戦柛婵嗗婢跺嫭鎱ㄥΟ绋垮缂侇噯缍佸顕€宕奸悢鍝勫箰濠电偠鎻紞鈧い顐㈩槸閻ｅ嘲鐣濋崟顒傚幍濡炪倖鐗楀銊╂倿閸涘ǹ浜滈柕蹇婂墲缁€瀣攽闄囬崺鏍箚閺冨牆顫呴柍钘夋閺佸綊姊婚崒姘偓椋庣矆娴ｈ櫣绀婂┑鐘叉硽婢舵劕绠婚悹鍥皺椤ρ冣攽椤斿浠滈柛瀣尵閳ь剚顔栭崳顕€宕戞繝鍥╁祦婵☆垰鍚嬬€氭岸鏌涘▎蹇ｆ▓婵☆偆鍠栧缁樼瑹閳ь剙顭囪閹囧幢濡炪垺绋戣灃闁告粈鐒﹂弲婊堟⒑閸撴彃浜濇繛鍙夛耿閹繝寮撮悙鈺傛杸闂佺粯蓱瑜板啴鍩€椤掆偓閻栫厧鐣烽幋锕€顫呴柕鍫濇閸樺憡绻涙潏鍓хК妞ゎ偄顦悾宄扮暆閳ь剟鍩€椤掍緡鍟忛柛锝庡櫍瀹曟粓鎮㈤梹鎰畾闂佸壊鍋呭ú鏍嵁閵忊€茬箚闁靛牆鎷戝妤冪磼閹插鐣垫慨濠傤煼瀹曟帒鈻庨幇顔哄仒婵犵數鍋涢ˇ鏉棵洪悢椋庢殾濞村吋娼欑粻娑㈡煛婢跺﹦浠㈤柛銈嗗灦缁绘繈鎮介棃娴讹綁鏌ら悷鏉库挃缂侇喖顭烽、娑㈡倷鐎电ǹ骞楅梻浣侯攰閹活亞寰婇崐鐕佹毐闂傚倷绀侀幖顐﹀箠韫囨稑绠栭柛宀€鍋涢拑鐔兼煥閻斿搫孝缁炬儳鍚嬫穱濠囶敍濠靛嫧鍋撻埀顒勬煕鐎ｎ偅宕岀€殿喕绮欓、鏇㈡偄閾氬倸顥氭繝娈垮枟閿曗晠宕滃☉姘辩煋婵炲樊浜濋悡娑㈡倶閻愰鍤欏┑顔煎€块弻鐔碱敊閸濆嫮浼勭紓渚囧枟閻熝囧箲閸曨垰惟闁挎棃鏁崑鎾活敇閵忊檧鎷哄┑顔炬嚀濞层倝鎮炲ú顏呯厱闁靛ǹ鍎崑銏⑩偓瑙勬礃濠㈡ǹ鐏冮梺鍛婂姧缁茶姤绂嶉悙顒佸弿婵せ鍋撶痪顓熸倐瀹曟繂顫㈠▔鍍盿b婵犵數濮烽弫鎼佸磻閻愬搫鍨傞柛顐ｆ礀缁犱即鏌熺紒銏犳灈缁炬儳顭烽弻鐔煎礈瑜忕敮娑㈡煃闁垮鐏撮柟顔肩秺楠炰線骞掗幋婵愮€抽梻浣告惈椤戝棝鎯勯鐐茶摕闁绘棁娅ｆす鎶芥倵閿濆倹娅婇柛瀣殜濮婄儤娼幍顔煎闂佸憡姊归…鍥╁垝鐠囨祴妲堥柕蹇曞У椤ユ繂鈹戦悙鍙夘棞鐟滄壆鍋熷Σ鎰版晝閸屾稈鎷婚梺绋挎湰閻燂妇绮婇悧鍫㈢濠㈣泛顑嗙粈瀣偓瑙勬磻閸楁娊鐛崶顒€绾ч柛顭戝枛濞堛倕鈹戦悩鍨毄濠殿喚鏁婚幊婵嬪礈瑜忔稉宥嗐亜閺嶎偄浠﹂柣鎾跺枛閺岀喐娼忛崜褍鍩岄悶姘哺濮婃椽宕崟顒€娅ら梺璇″枛閸婂灝顕ｆ繝姘櫜濠㈣埖蓱閺咃綁鎮峰⿰鍐ｇ紒鍌氱Ч閹瑥菐椤戣法鐩庨梻浣瑰缁诲倹顨ョ粙搴撴瀺鐎广儱妫▓浠嬫煟閹邦垰鐨虹紒鐘差煼閺岋紕浠︾拠鎻掝瀷闂侀€炲苯澧剧紓宥呮瀹曟垿骞掗幊绛圭秮椤㈡盯鎮欑划瑙勫濠电偠鎻紞鈧┑顔哄€楀☉鐢稿醇閺囩喓鍘遍梺缁樏崯鎸庢叏瀹ュ洠鍋撳▓鍨灈闁硅绱曢幑銏犫攽閸♀晜鍍靛銈嗗笒閸嬪棝宕悽鍛娾拻闁稿本鐟чˇ锕傛煟韫囨梻绠炴い銏＄墵瀹曞崬鈽夊Ο鏄忕发闂備礁澹婇崑渚€宕曢弻銉ョ厱闁硅揪闄勯崑锝夋煙椤撶喎绗掑┑鈥炽偢閺岋紕鈧綆鍋勯悘鎾煛瀹€瀣？闁逞屽墾缂嶅棙绂嶉崼鏇熷亗闁稿繒鈷堝▓浠嬫煟閹邦垰鐨虹紒鐘差煼閺岀喖顢欓悾宀€鐓夐梺鐟扮－閸嬨倖淇婇悜鑺ユ櫆缂佹稑顑勯幋鐑芥⒒閸屾艾鈧嘲霉閸パ€鏋栭柡鍥ュ灩闂傤垶鏌ㄩ弴鐐测偓鍝ョ不椤栫偞鐓ラ柣鏇炲€圭€氾拷1*/
void peakdet(const double * num,int N,double * indMax,int * indexMax,double * indMin,
             int * indexMin,int * max_len,int * min_len,double delta)
{
    double mn = 99999999., mx = -99999999.;
    int mnpos = 0, mxpos = 0;
    int lookformax = 1;
    double thi;
    //int a = 0;
    for(int i = 0;i < N;i++)
    {
        thi = num[i];
        if(thi > mx){
            mx = thi;
            mxpos = i;
        }
        if(thi < mn){
            mn = thi;
            mnpos = i;
        }

        if(lookformax){
            if(thi <  mx-delta){
                indexMax[*max_len] = mxpos;
                indMax[*max_len] = num[mxpos];
                (*max_len)++;
                mn = thi;
                mnpos = i;
                lookformax = 0;
            }
        }
        else{
            if(thi > mn+delta){
                indexMin[*min_len] = mnpos;
                indMin[*min_len] = num[mnpos];
                (*min_len)++;
                mx = thi;
                mxpos = i;
                lookformax = 1;
            }

        }
    }
}
void detect_local_interval_by_max(const double array[],double a[],int size,int Nmin,int Nmax)
{
    int center_p,temp;
    temp = Nmax-Nmin+1;
    center_p = size/2 - 1;
    int center = center_p +1;
    double * corr_vec = (double *)malloc(size*sizeof(double));

    for(int i = Nmin; i <= Nmax; i++)
    {
        double * seg_left = (double *)malloc(i*sizeof(double));
        double * seg_right = (double *)malloc(i*sizeof(double));
        int begin1 = center_p-i+1;
        int end1 = center_p+i;
        Data_transfer(seg_left,array, begin1, center);
        Data_transfer(seg_right,array, center_p, end1);
        corr_vec[i-Nmin] = accumulate2(seg_left, seg_right,i);
        free(seg_left);seg_left = NULL;
        free(seg_right);seg_right = NULL;
    }
    double max_value_index[2];
    max_v_interval(corr_vec,max_value_index, temp);
    free(corr_vec);corr_vec = NULL;
    double max_value = max_value_index[0];
    int max_index = max_value_index[1];
    int isend = 0;
    if(max_index == 1 || max_index == Nmax-Nmin+1){
        isend = 1;
    }
    max_index = Nmin+max_index - 1;
    a[0] = max_index;
    a[1] = max_value;
    a[2] = isend;
}
void detect_local_interval_by_peakdet(const double array[],double a[],int size,int Nmin,int Nmax)
{
    int center_p,temp;
    temp = Nmax-Nmin+1;
    center_p = size/2 - 1;
    int center = center_p +1;
    double * corr_vec = (double *)malloc(temp*sizeof(double));
    for(int i = Nmin; i <= Nmax; i++)
    {
        double * seg_left = (double *)malloc(i*sizeof(double));
        double * seg_right = (double *)malloc(i*sizeof(double));
        int begin1 = center_p-i+1;
        int end1 = center_p+i;
        Data_transfer(seg_left,array, begin1, center);
        Data_transfer(seg_right,array, center_p, end1);
        corr_vec[i-Nmin] = accumulate2(seg_left, seg_right,i);
        free(seg_left);seg_left = NULL;
        free(seg_right);seg_right = NULL;
    }
    double max_value_index[2];
    max_value_index[0] = 1.;
    max_value_index[1] = -1.;
    double * indMax = (double *)malloc(50*sizeof(double));
    int * indexMax = (int *)malloc(50*sizeof(int));
    double * indMin = (double *)malloc(50*sizeof(double));
    int * indexMin = (int *)malloc(50*sizeof(int));
    int max_len=0,min_len=0;
    double delta = 0.01;
    peakdet(corr_vec,temp,indMax,indexMax,indMin,indexMin,&max_len,&min_len,delta);
    if(max_len == 0){
        a[0] = max_value_index[0];
        a[1] = max_value_index[1];
        a[2] = 1;
        free(indMax);indMax = NULL;
        free(indexMax);indexMax = NULL;
        free(indMin);indMin = NULL;
        free(indexMin);indexMin = NULL;
		free(corr_vec); corr_vec = NULL;
        return;
    }
    for(int i = 0; i < max_len; i++){
        int offset_pos = indexMax[i] + 1 + Nmin - 1;
        if(offset_pos > 17 && offset_pos < 62){
            if(indMax[i] > max_value_index[1]){
                max_value_index[0] = indexMax[i] + 1;
                max_value_index[1] = indMax[i];
            }
        }
    }
    free(indMax);indMax = NULL;
    free(indexMax);indexMax = NULL;
    free(indMin);indMin = NULL;
    free(indexMin);indexMin = NULL;
    free(corr_vec); corr_vec = NULL;
    int isend = 0;
    if(max_value_index[0] == 1 || max_value_index[0] == Nmax-Nmin+1){
        isend = 1;
    }
    max_value_index[0] = Nmin + max_value_index[0] - 1;
    a[0] = max_value_index[0];
    a[1] = max_value_index[1];
    a[2] = isend;
}

void save_history_info(history_statistics his_stat_arr[], int arr_size,int *his_stat_arr_top, history_statistics cur_stat)
{
		assert(*his_stat_arr_top < arr_size && *his_stat_arr_top >= 0);

		his_stat_arr[*his_stat_arr_top].absmax = cur_stat.absmax;
		his_stat_arr[*his_stat_arr_top].seg_energy = cur_stat.seg_energy;
		his_stat_arr[*his_stat_arr_top].pr_diff = cur_stat.pr_diff;
		his_stat_arr[*his_stat_arr_top].resp_value = cur_stat.resp_value;
		his_stat_arr[*his_stat_arr_top].heart_value = cur_stat.heart_value;

		*his_stat_arr_top = (*his_stat_arr_top + 1)%arr_size;
}

double  get_history_info_sum_of_some_field(history_statistics his_stat_arr[], int arr_size, int begin, int end, uint8_t field)
{
		int i = 0;
		double fsum = 0.;
		assert(begin <= arr_size && begin >=0);
		assert(end <= arr_size && end >=0);
		assert(begin < end);
		assert(field < 5);
		for(i = begin; i < end;i++)
		{
			switch(field)
			{
				case 0:
					fsum += his_stat_arr[i].absmax;
					break;
				case 1:
					fsum += his_stat_arr[i].seg_energy;
					break;
				case 2:
					fsum += his_stat_arr[i].pr_diff;
					break;
				case 3:
					fsum += his_stat_arr[i].resp_value;
					break;
				case 4:
					fsum += his_stat_arr[i].heart_value;
					break;
				default:
					break;
			}
		}
		return fsum;
}
