#include "SceneManager.h"
#include "../Logger.h"
#include "../Context.h"
#include "BaseScene.h"
#include "../Input/InputManager.h"
#include "../GraphicsManager.h"
#include "../AssetManaging/TextureManager.h"
#include "../Sound/SoundService.h"

#define INPUT_MANAGER (InputManager::GetInstance())

namespace star 
{
	SceneManager* SceneManager::m_pSceneManager = nullptr;

	SceneManager::SceneManager( void )
		: m_ActiveScene(nullptr)
		, m_NewActiveScene(nullptr)
		, m_bSwitchingScene(false)
		, m_bInitialized(false)
		, m_bDestroyRequested(false)
		, m_bPauseRequested(false)
		, m_CurrentSceneName(EMPTY_STRING)
#ifndef _WIN32
		, mApplicationPtr(nullptr)
#endif
	{
	}

	SceneManager::~SceneManager(void)
	{
		m_SceneList.clear();
	}


	SceneManager* SceneManager::GetInstance()
	{
		if(m_pSceneManager == nullptr)
		{
			m_pSceneManager = new SceneManager();
		}
		return (m_pSceneManager);
	}

	BaseScene* SceneManager::GetScene(const tstring & name)
	{
		if(m_SceneList.find(name) != m_SceneList.end())
		{
			return (m_SceneList[name]);
		}
		return (nullptr);
	}

	bool SceneManager::SetActiveScene(const tstring & name)
	{
		if(m_CurrentSceneName == name)
		{
			return (true);
		}
		if(m_SceneList.find(name) != m_SceneList.end())
		{
			Logger::GetInstance()->Log(LogLevel::Info, _T("Scene ") + name + _T(" is now Active"));
			m_NewActiveScene = m_SceneList[name];
			m_bSwitchingScene = true;
			m_bInitialized = m_NewActiveScene->IsInitialized();
			m_CurrentSceneName = name;
			return (true);
		}
		else
		{
			m_bSwitchingScene = false;
			return (false);
		}
		return (true);
	}

	bool SceneManager::AddScene(const tstring & name, BaseScene* scene)
	{
		if ( m_SceneList.find(name) == m_SceneList.end() )
		{
			m_SceneList[name] = scene;
			Logger::GetInstance()->Log(LogLevel::Info, _T("Adding scene"));
		}
		else
		{
			Logger::GetInstance()->Log(LogLevel::Info, _T("Scene Already Exists"));
			return (false);
		}
		return (true);
	}

	bool SceneManager::RemoveScene(const tstring & name)
	{
		auto it = m_SceneList.find(name);
		if(it != m_SceneList.end())
		{
			m_SceneList.erase(it);
			return (true);
		}
		return (false);
	}

	bool SceneManager::InitializeCurScene(const Context& context)
	{
		if(m_bInitialized)
		{
			return (true);
		}
		if(m_NewActiveScene == nullptr)
		{
			return (false);
		}
		Logger::GetInstance()->Log(LogLevel::Info, _T("Initializing Scene :") + m_CurrentSceneName);
		m_NewActiveScene->BaseInitialize(context);
		m_bInitialized = m_NewActiveScene->IsInitialized();
		return (m_bInitialized);

	}

	status SceneManager::Update(const Context& context)
	{
		if(m_bDestroyRequested || m_bPauseRequested)
		{
			return (STATUS_OK);
		}

		if(m_bSwitchingScene)
		{
			if(!m_bInitialized)
			{
				InitializeCurScene(context);
			}
			m_ActiveScene = m_NewActiveScene;
			m_NewActiveScene = nullptr;
			m_bSwitchingScene = false;
			return (STATUS_OK);
		}

		if(m_ActiveScene != nullptr)
		{
			return (m_ActiveScene->BaseUpdate(context));
		}
		return (STATUS_OK);
	}


	status SceneManager::Draw()
	{
		if(m_bDestroyRequested || m_bPauseRequested)
		{
			return (STATUS_OK);
		}
		if(m_ActiveScene != nullptr)
		{
			m_ActiveScene->BaseDraw();
		}
		return (STATUS_OK);
	}

#ifndef _WIN32

	void SceneManager::processActivityEvent(int32 pCommand, android_app* pApplication)
	{
		if(m_ActiveScene == nullptr)
		{
			return;
		}
		mApplicationPtr = pApplication;
		switch(pCommand)
		{
		case APP_CMD_CONFIG_CHANGED:
			Logger::GetInstance()->Log(LogLevel::Info, _T("SceneManager : APP_CMD_CONFIG_CHANGED"));
			m_ActiveScene->OnConfigurationChanged();
			break;

		case APP_CMD_DESTROY:
			Logger::GetInstance()->Log(LogLevel::Info, _T("SceneManager : APP_CMD_DESTROY"));
			break;

		case APP_CMD_GAINED_FOCUS:
			Logger::GetInstance()->Log(LogLevel::Info, _T("SceneManager : APP_CMD_GAINED_FOCUS"));
			Activate();
			star::SoundService::GetInstance()->ResumeAllSound();
			break;

		case APP_CMD_LOST_FOCUS:
			Logger::GetInstance()->Log(LogLevel::Info, _T("SceneManager : APP_CMD_LOST_FOCUS"));
			star::SoundService::GetInstance()->PauseAllSound();
			DeActivate();
			break;

		case APP_CMD_LOW_MEMORY:
			Logger::GetInstance()->Log(LogLevel::Info, _T("SceneManager : APP_CMD_LOW_MEMORY"));
			break;

		case APP_CMD_PAUSE:
			Logger::GetInstance()->Log(LogLevel::Info, _T("SceneManager : APP_CMD_PAUSE"));
			m_bPauseRequested=true;
			star::SoundService::GetInstance()->PauseAllSound();
			break;

		case APP_CMD_RESUME:
			Logger::GetInstance()->Log(LogLevel::Info, _T("SceneManager : APP_CMD_RESUME"));
			m_bPauseRequested=false;
			star::SoundService::GetInstance()->ResumeAllSound();
			break;

		case APP_CMD_SAVE_STATE:
			Logger::GetInstance()->Log(LogLevel::Info, _T("SceneManager : APP_CMD_SAVE_STATE"));
			m_ActiveScene->OnSaveState(&mApplicationPtr->savedState,&mApplicationPtr->savedStateSize);
			break;

		case APP_CMD_START:
			Logger::GetInstance()->Log(LogLevel::Info, _T("SceneManager : APP_CMD_START"));
			break;

		case APP_CMD_STOP:
			Logger::GetInstance()->Log(LogLevel::Info, _T("SceneManager : APP_CMD_STOP"));
			//star::SoundService::GetInstance()->Stop();
			break;

		case APP_CMD_TERM_WINDOW:
			Logger::GetInstance()->Log(LogLevel::Info, _T("SceneManager : APP_CMD_TERM_WINDOW"));
			DeActivate();
			break;
		}
	}

	int32 SceneManager::processInputEvent(AInputEvent* pEvent)
	{
		//[TODO] Cast the input event to motionEvent and pass that type
		int32_t lEventType = AInputEvent_getType(pEvent);
		switch (lEventType)
		{
		case AINPUT_EVENT_TYPE_MOTION:
			switch (AInputEvent_getSource(pEvent))
			{
			case AINPUT_SOURCE_TOUCHSCREEN:
				INPUT_MANAGER->OnTouchEvent(pEvent);
				return (true);
			default:
				return (false);
			}
			break;

		case AINPUT_EVENT_TYPE_KEY:
			return (false);
		default:
			return (false);
		}
		return (false);
	}

	void SceneManager::Activate()
	{
		star::Logger::GetInstance()->Log(star::LogLevel::Info, _T("Going trough activate"));
		if(mApplicationPtr->window != nullptr)
		{
			if(m_ActiveScene->BaseOnActivate() != STATUS_OK)
			{
				ANativeActivity_finish(mApplicationPtr->activity);
			}
		}
	}

	void SceneManager::DeActivate()
	{
		m_ActiveScene->BaseOnDeactivate();
		star::Logger::GetInstance()->Log(star::LogLevel::Info, _T("Going trough DeActivate"));
		star::GraphicsManager::GetInstance()->Destroy();
		star::TextureManager::GetInstance()->EraseTextures();
		//star::SoundService::GetInstance()->Stop();
		m_SceneList.clear();
		m_CurrentSceneName = EMPTY_STRING;
	}
#endif // _WIN32
}
