// ==============================
// 把原来CGame中关于脚本的代码独立出来
// jollywing@foxmail.com, 2016-07-29
// ==============================

#include <string.h>
#include "CGame.h"
#include "ScriptEngine.h"
#include "TextParser.h"

ScriptEngine::ScriptEngine()
{
    pScripts = NULL;
	nCurLineIndex = 0;
	nScriptLineNumber = 0;
}

ScriptEngine::~ScriptEngine()
{
    if( pScripts )
	{
		delete [] pScripts;
		pScripts = NULL;
	}

}

/* load scripts */
bool ScriptEngine::LoadScripts( char * pstFileName, char cHeadFlag )
{
	if( pScripts )
	{
		delete [] pScripts;
		pScripts = NULL;
	}

	CTextParser ScriptsFile( pstFileName, cHeadFlag);	//open file
	nScriptsNumber = ScriptsFile.GetRecordNum();

	char stTemp[ 512 ];
	short nScriptsIndex = -1;
	short nValidLineIndex = -1;
	short nLineNumber = 0;
	short nValidLineNum = 0;

	//calculate line number
	while( ScriptsFile.GetWholeLine(stTemp))
	{
		nLineNumber ++;

		if( stTemp[0] != cHeadFlag )
			nValidLineNum ++ ;
	}

	nScriptLineNumber = nValidLineNum;

	//malloc memory space to hold scripts
	pScripts = new CScriptLine [ nValidLineNum ];
	if ( !pScripts )
	{
		cerr << "memory not enough!" << endl;
		return false;
	}

	// read scripts into memory
	ScriptsFile.GotoFileHead();
    short i;
	for ( i = 0; i < nLineNumber; ++ i )
	{
		ScriptsFile.GetWholeLine(stTemp);
		if(stTemp[0] == cHeadFlag)
		{
			nScriptsIndex ++;
		}
		else
		{
			nValidLineIndex ++;
			if ( nValidLineIndex < nValidLineNum )
			{
				pScripts[ nValidLineIndex ].nScriptIndex = nScriptsIndex;
				strcpy( pScripts[ nValidLineIndex ].stScriptLine,
					stTemp );
			}

		}
	}
    return true;
}

/* given a id, find the first scripts who have the same id */
bool ScriptEngine::FindScripts( short nScriptsID )
{
	if ( !pScripts || nScriptsID >= nScriptsNumber || nScriptsID < 0 )
	{
		return false;
	}

	nCurLineIndex = 0;

	while ( pScripts [ nCurLineIndex ].nScriptIndex < nScriptsID )
	{
		nCurLineIndex ++;
	}

	return true;
}

/* run scripts */
void ScriptEngine::RunScripts( short nScriptID, CGame *pGameEngine )
{
	if(! FindScripts( nScriptID ) )
		return;

    iRunningScripts = true;
	while( iRunningScripts )
	{
		//execute scripte line
		ExecuteScriptLine( pScripts[ nCurLineIndex ].stScriptLine, pGameEngine );
		nCurLineIndex ++;
	}
}

/* 执行一条脚本语句 */
void ScriptEngine::ExecuteScriptLine( char * stScriptLine, CGame *pGameEngine )
{
	nIndexInCurLine = 0;

	// 读取命令
	char stCommand[ 32 ];
	ReadSubString( stScriptLine, stCommand );

	char stStringBuffer[256];
	char stNumberBuffer[4];

    // 返回
	if (! strcmp( stCommand, "RETURN" )) {
        iRunningScripts = false;
    }

	//屏幕输出
	else if (! strcmp( stCommand, "PRINT" ))
	{
		ReadSubString( stScriptLine, stStringBuffer );
		pGameEngine->SystemOut( stStringBuffer );
		pGameEngine->SystemPause();
	}

	//暂停
	else if( ! strcmp( stCommand, "PAUSE") )
		pGameEngine->SystemPause();

	else if( ! strcmp( stCommand, "CLEAR") )
        system(CLEAR);

	//讲话
	else if ( ! strcmp( stCommand, "TALK" ) )
	{
		//读取NPC的索引
		ReadSubString( stScriptLine, stNumberBuffer );
		short nNpcIndex = atoi( stNumberBuffer );

		//读取讲话内容
		ReadSubString(stScriptLine, stStringBuffer );

		//输出
		pGameEngine->ShowNpcName( nNpcIndex );
		cout << ": ";
#ifdef _WIN32_
		short nRandomColor = 1 + rand()%15;
		SetConsoleTextAttribute( hConsoleHandle, nRandomColor|0);
		cout << stStringBuffer<<endl << endl;
#else
        printf(SYSTEM_HINT_STRING, stStringBuffer);
        cout << endl;
#endif
		pGameEngine->SystemPause();
	}

	//测试某个任务值，如果等于测试值则脚本跳转到指定地方
	else if ( ! strcmp( stCommand, "QUEST_JUMP" ) )
	{
		//读取任务索引
		ReadSubString( stScriptLine, stNumberBuffer);
		short nQuestIndex = atoi( stNumberBuffer );

		//读取测试值
		ReadSubString( stScriptLine, stNumberBuffer);
		short nTestValue = atoi( stNumberBuffer );

		//如果测试成立则跳转
		if ( pGameEngine->pQuests[nQuestIndex].nFlagValue == nTestValue )
		{
			//读取跳转的脚本索引
			ReadSubString( stScriptLine, stNumberBuffer);
			short nScriptIndex = atoi( stNumberBuffer );

			//跳转
			FindScripts( nScriptIndex );
			nCurLineIndex --;
		}
	}

	//设置任务标志
	else if( ! strcmp(stCommand, "SET_FLAG") )
	{
		//读取任务索引
		ReadSubString( stScriptLine, stNumberBuffer);
		short nQuestIndex = atoi( stNumberBuffer );

		//读取设定值
		ReadSubString( stScriptLine, stNumberBuffer);
		short nSetValue = atoi( stNumberBuffer );

		pGameEngine->pQuests[nQuestIndex].nFlagValue = nSetValue;
	}

    //如果角色身上拥有指定数量的某道具
    else if( ! strcmp(stCommand, "HAVE_JUMP") ){
		ReadSubString( stScriptLine, stNumberBuffer);
		short nRoleIdx = atoi( stNumberBuffer );
		ReadSubString( stScriptLine, stNumberBuffer);
		short nGoodsIdx = atoi( stNumberBuffer );
		ReadSubString( stScriptLine, stNumberBuffer);
		short nNumber = atoi( stNumberBuffer );
		if ( pGameEngine->NumOfGoods(nRoleIdx, nGoodsIdx) >= nNumber){
			ReadSubString( stScriptLine, stNumberBuffer);
			short nScriptIndex = atoi( stNumberBuffer );
			FindScripts( nScriptIndex );
			nCurLineIndex --;
		}
    }

    //如果变量大于等于则跳转
    else if( ! strcmp(stCommand, "MORE_JUMP") ){
		ReadSubString( stScriptLine, stNumberBuffer);
		short nVarIndex = atoi( stNumberBuffer );
		ReadSubString( stScriptLine, stNumberBuffer);
		short nTestValue = atoi( stNumberBuffer );
		if ( pGameEngine->nVars[nVarIndex] >= nTestValue ){
			ReadSubString( stScriptLine, stNumberBuffer);
			short nScriptIndex = atoi( stNumberBuffer );
			FindScripts( nScriptIndex );
			nCurLineIndex --;
		}
    }

    //如果变量等于则跳转
    else if( ! strcmp(stCommand, "EQ_JUMP") ){
		ReadSubString( stScriptLine, stNumberBuffer);
		short nVarIndex = atoi( stNumberBuffer );
		ReadSubString( stScriptLine, stNumberBuffer);
		short nTestValue = atoi( stNumberBuffer );
		if ( pGameEngine->nVars[nVarIndex] == nTestValue ){
			ReadSubString( stScriptLine, stNumberBuffer);
			short nScriptIndex = atoi( stNumberBuffer );
			FindScripts( nScriptIndex );
			nCurLineIndex --;
		}
    }

    //设定变量
    else if( ! strcmp(stCommand, "SET_VAR") ){
        ReadSubString( stScriptLine, stNumberBuffer);
        short nVarIndex = atoi( stNumberBuffer );
        ReadSubString( stScriptLine, stNumberBuffer);
        short nValue = atoi( stNumberBuffer );
        pGameEngine->nVars[nVarIndex] = nValue;
    }

    //增加变量
    else if( ! strcmp(stCommand, "ADD_VAR") ){
        ReadSubString( stScriptLine, stNumberBuffer);
        short nVarIndex = atoi( stNumberBuffer );
        pGameEngine->nVars[nVarIndex] = pGameEngine->nVars[nVarIndex] + 1;
    }

    // YES/NO
    else if( ! strcmp(stCommand, "YES_NO") ){
        pGameEngine->SystemOut("1.是  2.否");
        short nSel = pGameEngine->GetUserInput(1,2);
        pGameEngine->nVars[0] = nSel;
    }


	//继续冒险
	else if( ! strcmp (stCommand, "CONTINUE") )
		pGameEngine->oGameEnv.eState = player_adven;

	else if( ! strcmp( stCommand, "TRADE") )
	{
		pGameEngine->oGameEnv.eState = trading;
	}

	//战斗
	else if( ! strcmp (stCommand, "FIGHT") )
	{
		//读取NPC索引
		ReadSubString( stScriptLine, stNumberBuffer);
		short nNpcIndex = atoi( stNumberBuffer );

		//初始化战斗
		pGameEngine->InitFight(nNpcIndex);
	}

	else if( ! strcmp( stCommand, "ADD_GOODS") )
	{
		ReadSubString( stScriptLine, stNumberBuffer );
		short nRoleIdx = atoi( stNumberBuffer );
		ReadSubString( stScriptLine, stNumberBuffer );
		short nGoodsIndex = atoi( stNumberBuffer );
		pGameEngine->AddGoods( nRoleIdx, nGoodsIndex, true );
	}

    else if( ! strcmp( stCommand, "DEL_GOODS") ){
		ReadSubString( stScriptLine, stNumberBuffer );
		short nRoleIdx = atoi( stNumberBuffer );
		ReadSubString( stScriptLine, stNumberBuffer );
		short nGoodsIdx = atoi( stNumberBuffer );
        pGameEngine->RemoveGoods(nRoleIdx,
                pGameEngine->HaveGoods(nRoleIdx, nGoodsIdx), true);
    }

	else if( ! strcmp( stCommand, "ADD_MONEY") )
	{
		ReadSubString( stScriptLine, stNumberBuffer );
		short nRoleIdx = atoi( stNumberBuffer );
		ReadSubString( stScriptLine, stNumberBuffer );
		short nMoney = atoi( stNumberBuffer );
        pGameEngine->AddMoney(nRoleIdx, nMoney);
	}

	else if( ! strcmp( stCommand, "DEL_MONEY") )
	{
		ReadSubString( stScriptLine, stNumberBuffer );
		short nRoleIdx = atoi( stNumberBuffer );
		ReadSubString( stScriptLine, stNumberBuffer );
		short nMoney = atoi( stNumberBuffer );
        if(pGameEngine->DelMoney(nRoleIdx, nMoney))
            pGameEngine->nVars[1] = 0;  //success
        else
            pGameEngine->nVars[1] = 1;  //fail
	}

	//向指定地图添加npc
	else if ( ! strcmp( stCommand, "ADD_NPC" ) )
	{
		//读取地图索引
		ReadSubString( stScriptLine, stNumberBuffer);
		short nSpotIndex = atoi( stNumberBuffer );

		//读取npc索引
		ReadSubString( stScriptLine, stNumberBuffer );
		short nNpcIndex = atoi( stNumberBuffer );

		//添加
		if ( !pGameEngine->AddNpc( nSpotIndex, nNpcIndex) )
		{
			pGameEngine->SystemWarning("添加NPC失败，游戏将退出！");
			pGameEngine->iRunning = false;
		}
	}

	//移除指定地图的npc
	else if ( ! strcmp( stCommand, "REMOVE_NPC" ) )
	{
		//读取地图索引
		ReadSubString( stScriptLine, stNumberBuffer);
		short nSpotIndex = atoi( stNumberBuffer );

		//读取npc索引
		ReadSubString( stScriptLine, stNumberBuffer );
		short nNpcIndex = atoi( stNumberBuffer );

		//添加
		if ( !pGameEngine->RemoveNpc( nSpotIndex, nNpcIndex) )
		{
			pGameEngine->SystemWarning("删除NPC出错，游戏将退出！");
			pGameEngine->iRunning = false;
		}
	}

    else if(! strcmp(stCommand, "RANDOM_DROP")){
        // read goods index
		ReadSubString( stScriptLine, stNumberBuffer);
		short nGoodsIndex = atoi( stNumberBuffer );

        // read probability
		ReadSubString( stScriptLine, stNumberBuffer);
		short nProb = atoi( stNumberBuffer );

        pGameEngine->RandomDrop(pGameEngine->oGameEnv.nCurrentNpc, nGoodsIndex, nProb);
    }

    else if(! strcmp(stCommand, "REFRESH")){
		ReadSubString( stScriptLine, stNumberBuffer);
		short nRoleIdx = atoi( stNumberBuffer );
        pGameEngine->Refresh(nRoleIdx);
    }
}

/* read a sub string from a string */
void ScriptEngine::ReadSubString( char * stString, char * stSubString )
{
	short nIndex = 0;

	//ignore space before valid char
	while( stString[ nIndexInCurLine ] == ' ' || stString [ nIndexInCurLine ] == '\t')
	{
		nIndexInCurLine ++;
	}

	//read sub string
	while ( nIndexInCurLine < ( short )strlen(stString) )
	{
		if ( stString[ nIndexInCurLine] == ' ' ||
			stString[ nIndexInCurLine ] == '\t' ||
             stString[ nIndexInCurLine ] == '\r')
			break;
		else
		{
			stSubString[ nIndex ] = stString[ nIndexInCurLine ];
			nIndex ++;
		}
		nIndexInCurLine ++;
	}

	stSubString[ nIndex ] = '\0';

}
