package org.cryptomator.windows.uiappearance;

import org.cryptomator.integrations.uiappearance.Theme;
import org.cryptomator.integrations.uiappearance.UiAppearanceException;
import org.cryptomator.integrations.uiappearance.UiAppearanceListener;
import org.cryptomator.integrations.uiappearance.UiAppearanceProvider;

import javax.swing.*;
import java.util.ArrayList;
import java.util.Collection;

public class WinUiAppearanceProvider implements UiAppearanceProvider, WinAppearanceListener {

	private final WinAppearance winAppearance;
	private final Collection<UiAppearanceListener> registeredListeners;

	public WinUiAppearanceProvider() {
		this.winAppearance = new WinAppearance();
		this.registeredListeners = new ArrayList<>();
	}

	@Override
	public Theme getSystemTheme() {
		return winAppearance.getSystemTheme();
	}

	@Override
	public void adjustToTheme(Theme theme) {
		switch (theme) {
			case LIGHT:
				winAppearance.setToLight();
				break;
			case DARK:
				winAppearance.setToDark();
				break;
		}	}

	@Override
	public synchronized void addListener(UiAppearanceListener listener) {
		var wasEmpty = registeredListeners.isEmpty();
		registeredListeners.add(listener);
		if (wasEmpty) {
			winAppearance.startObserving(this);
		}
	}

	@Override
	public synchronized void removeListener(UiAppearanceListener listener) {
		registeredListeners.remove(listener);
		if (registeredListeners.isEmpty()) {
			winAppearance.stopObserving();
		}
	}

	//called from native code, to notify all observes of latest change
	@Override
	public void systemAppearanceChanged() {
		var currentTheme = getSystemTheme();
		for (var listener : registeredListeners) {
			listener.systemAppearanceChanged(currentTheme);
		}
	}
}
