/* NickServ core functions
 *
 * (C) 2003-2011 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

/*************************************************************************/

#include "module.h"
#include "nickserv.h"

class CommandNSGhost : public Command
{
 public:
	CommandNSGhost() : Command("GHOST", 1, 2)
	{
		this->SetFlag(CFLAG_ALLOW_UNREGISTERED);
		this->SetDesc(_("Disconnects a \"ghost\" IRC session using your nick"));
	}

	CommandReturn Execute(CommandSource &source, const std::vector<Anope::string> &params)
	{
		const Anope::string &nick = params[0];
		const Anope::string &pass = params.size() > 1 ? params[1] : "";

		User *u = source.u;
		User *user = finduser(nick);
		NickAlias *na = findnick(nick);

		if (!user)
			source.Reply(_(NICK_X_NOT_IN_USE), nick.c_str());
		else if (!na)
			source.Reply(_(NICK_X_NOT_REGISTERED), nick.c_str());
		else if (na->HasFlag(NS_FORBIDDEN))
			source.Reply(_(NICK_X_FORBIDDEN), na->nick.c_str());
		else if (na->nc->HasFlag(NI_SUSPENDED))
			source.Reply(_(NICK_X_SUSPENDED), na->nick.c_str());
		else if (nick.equals_ci(u->nick))
			source.Reply(_("You can't ghost yourself!"));
		else
		{
			bool ok = false;
			if (u->Account() == na->nc)
				ok = true;
			else if (!na->nc->HasFlag(NI_SECURE) && is_on_access(u, na->nc))
				ok = true;
			else if (!u->fingerprint.empty() && na->nc->FindCert(u->fingerprint))
				ok = true;
			else if (!pass.empty())
			{
				EventReturn MOD_RESULT;
				FOREACH_RESULT(I_OnCheckAuthentication, OnCheckAuthentication(u, this, params, na->nc->display, pass));
				if (MOD_RESULT == EVENT_STOP)
					return MOD_CONT;
				else if (MOD_RESULT == EVENT_ALLOW)
					ok = true;
			}

			if (ok)
			{
				if (!user->IsIdentified() && FindCommand(nickserv->Bot(), "RECOVER"))
					source.Reply(_("You may not ghost an unidentified user, use RECOVER instead."));
				else
				{
					Log(LOG_COMMAND, u, this) << "for " << nick;
					Anope::string buf = "GHOST command used by " + u->nick;
					user->Kill(Config->s_NickServ, buf);
					source.Reply(_("Ghost with your nick has been killed."), nick.c_str());
				}
			}
			else
			{
				source.Reply(_(ACCESS_DENIED));
				if (!pass.empty())
				{
					Log(LOG_COMMAND, u, this) << "with an invalid password for " << nick;
					bad_password(u);
				}
			}
		}

		return MOD_CONT;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand)
	{
		source.Reply(_("Syntax: \002GHOST \037nickname\037 [\037password\037]\002\n"
				""
				"itermminates a \"ghost\" IRC session using your nick.  A\n"
				"ghost\" session is one which is not actually connected,\n"
				"but which the IRC server believes is still online for one\n"
				"reason or another. Typically, this happens if your\n"
				"computer crashes or your Internet or modem connection\n"
				"goes down while you're on IRC.\n"
				" \n"
				"In order to use the \002GHOST\002 command for a nick, your\n"
				"current address as shown in /WHOIS must be on that nick's\n"
				"access list, you must be identified and in the group of\n"
				"that nick, or you must supply the correct password for\n"
				"the nickname."));
		return true;
	}

	void OnSyntaxError(CommandSource &source, const Anope::string &subcommand)
	{
		SyntaxError(source, "GHOST", _("GHOST \037nickname\037 [\037password\037]"));
	}
};

class NSGhost : public Module
{
	CommandNSGhost commandnsghost;

 public:
	NSGhost(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, CORE)
	{
		this->SetAuthor("Anope");

		if (!nickserv)
			throw ModuleException("NickServ is not loaded!");

		this->AddCommand(nickserv->Bot(), &commandnsghost);
	}
};

MODULE_INIT(NSGhost)
