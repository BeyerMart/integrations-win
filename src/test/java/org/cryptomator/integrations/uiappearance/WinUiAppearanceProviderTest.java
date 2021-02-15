package org.cryptomator.integrations.uiappearance;

import org.cryptomator.windows.uiappearance.WinUiAppearanceProvider;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

public class WinUiAppearanceProviderTest {
	private WinUiAppearanceProvider appearanceProvider;
	private Theme changedTheme;

	@BeforeEach
	public void setup() {
		this.appearanceProvider = new WinUiAppearanceProvider();
	}

	@Test
	public void testGettingTheCurrentTheme(){
		System.out.println(appearanceProvider.getSystemTheme().toString());
	}

	@Test
	public void testRegisterAndUnregisterObserver() {
		UiAppearanceListener listener = theme -> System.out.println(theme.toString());
		appearanceProvider.addListener(listener);
		try {
			Thread.sleep(500);//Sleep for a time until a hidden window is built
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		appearanceProvider.removeListener(listener);
	}

	@Test //Warning: Sets your System Theme
	public void testRecogniseAChangedAppModeTheme() {
		UiAppearanceListener listener = theme -> {
			changedTheme = theme;
		};
		appearanceProvider.addListener(listener);
		try {
			Thread.sleep(500);//Sleep for a time until a hidden window is built
			appearanceProvider.adjustToTheme(Theme.LIGHT);
			Thread.sleep(1500);
			if (changedTheme != null) { //changedTheme is NULL when the theme already was LIGHT (so no change occured)
				Assertions.assertEquals(Theme.LIGHT, changedTheme);
			}
			appearanceProvider.adjustToTheme(Theme.DARK);
			Thread.sleep(1500);
			Assertions.assertEquals(Theme.DARK, changedTheme);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		appearanceProvider.removeListener(listener);
	}
}
