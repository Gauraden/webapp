--- Разбор стека вызовов -------------------------------------------------------
Server::Run:
  Resources::HandleAccept:
    Resources::WaitForConnection: инициализаия ожидания следующего подключения
    Session::WaitForRequest     : обработка текущего подключения
      Session::ReadHandler:
        
